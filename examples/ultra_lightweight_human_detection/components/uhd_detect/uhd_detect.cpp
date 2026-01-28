#include "uhd_detect.hpp"

#include "dl_image_preprocessor.hpp"
#include "dl_tensor_base.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "uhd_constants.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <type_traits>

namespace {
constexpr char kTag[] = "uhd_detect";

#ifndef UHD_MODEL_SYMBOL
#define UHD_MODEL_SYMBOL ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nocat_espdl
#endif
#ifndef UHD_MODEL_NAME
#define UHD_MODEL_NAME "uhd_model"
#endif

#define SYMBOL_JOIN(a, b) a##b
#define SYMBOL_MAKE(a, b) SYMBOL_JOIN(a, b)
#define UHD_MODEL_START SYMBOL_MAKE(SYMBOL_MAKE(_binary_, UHD_MODEL_SYMBOL), _start)
#define UHD_MODEL_END SYMBOL_MAKE(SYMBOL_MAKE(_binary_, UHD_MODEL_SYMBOL), _end)

extern "C" {
extern const uint8_t UHD_MODEL_START[];
extern const uint8_t UHD_MODEL_END[];
}

struct ModelBlob {
    const uint8_t *data = nullptr;
    size_t size = 0;
    bool owned = false;
};

ModelBlob load_model_blob()
{
    ModelBlob blob;
    const uint8_t *start = UHD_MODEL_START;
    const uint8_t *end = UHD_MODEL_END;
    if (!start || !end || end <= start) {
        ESP_LOGE(kTag, "model symbol is invalid");
        return blob;
    }

    blob.size = static_cast<size_t>(end - start);
    blob.data = start;

    uintptr_t addr = reinterpret_cast<uintptr_t>(start);
    if ((addr & 0x0F) != 0) {
        uint8_t *aligned = static_cast<uint8_t *>(
            heap_caps_aligned_alloc(16, blob.size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!aligned) {
            aligned = static_cast<uint8_t *>(heap_caps_aligned_alloc(16, blob.size, MALLOC_CAP_8BIT));
        }
        if (!aligned) {
            ESP_LOGE(kTag, "failed to allocate aligned model buffer (%zu bytes)", blob.size);
            blob.data = nullptr;
            blob.size = 0;
            return blob;
        }
        std::memcpy(aligned, start, blob.size);
        blob.data = aligned;
        blob.owned = true;
        ESP_LOGW(kTag, "model data copied to aligned buffer");
    }

    return blob;
}

static inline float sigmoid_f(float x)
{
    if (x >= 80.0f) {
        return 1.0f;
    }
    if (x <= -80.0f) {
        return 0.0f;
    }
    return 1.0f / (1.0f + std::exp(-x));
}

static inline float softplus_f(float x)
{
    if (x > 20.0f) {
        return x;
    }
    if (x < -20.0f) {
        return std::exp(x);
    }
    return std::log1p(std::exp(x));
}

class UhdLitePostprocessor : public dl::detect::DetectPostprocessor {
public:
    UhdLitePostprocessor(dl::Model *model,
                         dl::image::ImagePreprocessor *image_preprocessor,
                         float score_thr,
                         float nms_thr,
                         int top_k) :
        dl::detect::DetectPostprocessor(model, image_preprocessor, score_thr, nms_thr, top_k)
    {
        if (!uhd_detect::get_uhd_anchor_set(UHD_MODEL_NAME, &m_anchor_set)) {
            m_anchor_set = {nullptr, nullptr, 0};
        }
    }

    void postprocess() override
    {
        dl::TensorBase *box = m_model->get_output("box");
        dl::TensorBase *quality = m_model->get_output("quality");
        if (!box || !quality) {
            ESP_LOGE(kTag, "missing output tensor(s)");
            return;
        }

        if (box->shape.size() != 4 || quality->shape.size() != 4) {
            ESP_LOGE(kTag, "unexpected output dims");
            return;
        }

        bool nhwc = (box->shape[3] % 4 == 0) && (quality->shape[3] == box->shape[3] / 4);
        bool nchw = (box->shape[1] % 4 == 0) && (quality->shape[1] == box->shape[1] / 4);
        if (!nhwc && !nchw) {
            ESP_LOGE(kTag, "output tensor shape mismatch (box=%s quality=%s)",
                     dl::vector_to_string(box->shape).c_str(),
                     dl::vector_to_string(quality->shape).c_str());
            return;
        }

        if (!m_anchor_set.anchors || !m_anchor_set.wh_scale || m_anchor_set.count <= 0) {
            ESP_LOGE(kTag, "anchors/wh_scale missing for model %s", UHD_MODEL_NAME);
            return;
        }

        if (box->dtype == dl::DATA_TYPE_INT8) {
            parse_maps<int8_t>(box, quality, nhwc);
        } else if (box->dtype == dl::DATA_TYPE_INT16) {
            parse_maps<int16_t>(box, quality, nhwc);
        } else if (box->dtype == dl::DATA_TYPE_FLOAT) {
            parse_maps<float>(box, quality, nhwc);
        } else {
            ESP_LOGE(kTag, "unsupported output dtype: %s", dl::dtype_to_string(box->dtype));
            return;
        }

        nms();
    }

private:
    template <typename T>
    void parse_maps(dl::TensorBase *box, dl::TensorBase *quality, bool nhwc)
    {
        const int H = nhwc ? box->shape[1] : box->shape[2];
        const int W = nhwc ? box->shape[2] : box->shape[3];
        const int na = nhwc ? quality->shape[3] : quality->shape[1];
        const int box_c = nhwc ? box->shape[3] : box->shape[1];
        if (box_c != na * 4) {
            ESP_LOGE(kTag, "box channel mismatch: box_c=%d anchors=%d", box_c, na);
            return;
        }
        if (na != m_anchor_set.count) {
            ESP_LOGW(kTag, "anchor count mismatch: model=%d const=%d", na, m_anchor_set.count);
        }

        auto *box_ptr = static_cast<T *>(box->data);
        auto *quality_ptr = static_cast<T *>(quality->data);

        const bool is_float = std::is_floating_point_v<T>;
        const float box_scale = is_float ? 1.f : DL_SCALE(box->exponent);
        const float quality_scale = is_float ? 1.f : DL_SCALE(quality->exponent);

        float inv_resize_scale_x = m_image_preprocessor->get_resize_scale_x(true);
        float inv_resize_scale_y = m_image_preprocessor->get_resize_scale_y(true);
        int top_left_x = m_image_preprocessor->get_crop_area_top_left_x();
        int top_left_y = m_image_preprocessor->get_crop_area_top_left_y();
        dl::TensorBase *input = m_image_preprocessor->get_model_input();
        float scale_w = static_cast<float>(input->shape[2]) * inv_resize_scale_x;
        float scale_h = static_cast<float>(input->shape[1]) * inv_resize_scale_y;

        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                for (int a = 0; a < na; ++a) {
                    size_t idx_q = nhwc ? ((static_cast<size_t>(y) * W + x) * na + a)
                                        : ((static_cast<size_t>(a) * H + y) * W + x);
                    float q_raw = is_float ? static_cast<float>(quality_ptr[idx_q])
                                           : dl::dequantize(quality_ptr[idx_q], quality_scale);
                    float score = sigmoid_f(q_raw);
                    if (score < m_score_thr) {
                        continue;
                    }

                    size_t idx_b = nhwc ? ((static_cast<size_t>(y) * W + x) * na * 4 + a * 4)
                                        : ((static_cast<size_t>(a) * 4) * H * W + (static_cast<size_t>(y) * W + x));
                    float tx = is_float ? static_cast<float>(box_ptr[idx_b + (nhwc ? 0 : 0 * H * W)])
                                        : dl::dequantize(box_ptr[idx_b + (nhwc ? 0 : 0 * H * W)], box_scale);
                    float ty = is_float ? static_cast<float>(box_ptr[idx_b + (nhwc ? 1 : 1 * H * W)])
                                        : dl::dequantize(box_ptr[idx_b + (nhwc ? 1 : 1 * H * W)], box_scale);
                    float tw = is_float ? static_cast<float>(box_ptr[idx_b + (nhwc ? 2 : 2 * H * W)])
                                        : dl::dequantize(box_ptr[idx_b + (nhwc ? 2 : 2 * H * W)], box_scale);
                    float th = is_float ? static_cast<float>(box_ptr[idx_b + (nhwc ? 3 : 3 * H * W)])
                                        : dl::dequantize(box_ptr[idx_b + (nhwc ? 3 : 3 * H * W)], box_scale);

                    float anchor_w = m_anchor_set.anchors[a * 2] * m_anchor_set.wh_scale[a * 2];
                    float anchor_h = m_anchor_set.anchors[a * 2 + 1] * m_anchor_set.wh_scale[a * 2 + 1];

                    float cx = (sigmoid_f(tx) + static_cast<float>(x)) / static_cast<float>(W);
                    float cy = (sigmoid_f(ty) + static_cast<float>(y)) / static_cast<float>(H);
                    float bw = anchor_w * softplus_f(tw);
                    float bh = anchor_h * softplus_f(th);

                    float x1 = (cx - 0.5f * bw) * scale_w + static_cast<float>(top_left_x);
                    float y1 = (cy - 0.5f * bh) * scale_h + static_cast<float>(top_left_y);
                    float x2 = (cx + 0.5f * bw) * scale_w + static_cast<float>(top_left_x);
                    float y2 = (cy + 0.5f * bh) * scale_h + static_cast<float>(top_left_y);

                    if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2)) {
                        continue;
                    }
                    if (x2 <= x1 || y2 <= y1) {
                        continue;
                    }

                    dl::detect::result_t new_box = {0, score, {(int)x1, (int)y1, (int)x2, (int)y2}, {}};
                    m_box_list.insert(
                        std::upper_bound(m_box_list.begin(), m_box_list.end(), new_box, dl::detect::greater_box),
                        new_box);
                }
            }
        }
    }

    uhd_detect::UhdAnchorSet m_anchor_set;
};
} // namespace

namespace uhd_detect {
UltraLightweightHumanDetect::UltraLightweightHumanDetect(float score_thr, float nms_thr, int top_k)
{
    m_model = nullptr;
    m_image_preprocessor = nullptr;
    m_postprocessor = nullptr;

    ModelBlob blob = load_model_blob();
    if (!blob.data || blob.size == 0) {
        ESP_LOGE(kTag, "model blob is empty");
        return;
    }

    ESP_LOGI(kTag, "model=%s", UHD_MODEL_NAME);

    m_model_data = blob.data;
    m_model_owned = blob.owned;

    m_model = new dl::Model(reinterpret_cast<const char *>(m_model_data),
                            fbs::MODEL_LOCATION_IN_FLASH_RODATA,
                            0,
                            dl::MEMORY_MANAGER_GREEDY,
                            nullptr,
                            true);
    if (!m_model) {
        ESP_LOGE(kTag, "model allocation failed");
        return;
    }

    m_model->minimize();

#if CONFIG_IDF_TARGET_ESP32P4
    uint32_t caps = dl::image::DL_IMAGE_CAP_RGB_SWAP;
#else
    uint32_t caps = dl::image::DL_IMAGE_CAP_RGB_SWAP | dl::image::DL_IMAGE_CAP_RGB565_BIG_ENDIAN;
#endif

    m_image_preprocessor = new dl::image::ImagePreprocessor(m_model, {0, 0, 0}, {255, 255, 255}, caps);
    m_postprocessor = new UhdLitePostprocessor(m_model, m_image_preprocessor, score_thr, nms_thr, top_k);
}

UltraLightweightHumanDetect::~UltraLightweightHumanDetect()
{
    if (m_model_owned && m_model_data) {
        heap_caps_free(const_cast<uint8_t *>(m_model_data));
        m_model_data = nullptr;
        m_model_owned = false;
    }
}

std::list<dl::detect::result_t> &UltraLightweightHumanDetect::run(const dl::image::img_t &img)
{
    if (!m_model || !m_image_preprocessor || !m_postprocessor) {
        static std::list<dl::detect::result_t> empty;
        ESP_LOGE(kTag, "model not initialized");
        return empty;
    }

    int64_t t0 = esp_timer_get_time();
    m_image_preprocessor->preprocess(img);
    int64_t t1 = esp_timer_get_time();

    m_model->run();
    int64_t t2 = esp_timer_get_time();

    m_postprocessor->clear_result();
    m_postprocessor->postprocess();
    std::list<dl::detect::result_t> &result = m_postprocessor->get_result(img.width, img.height);
    int64_t t3 = esp_timer_get_time();

    ESP_LOGI(kTag,
             "latency(ms): pre=%.2f infer=%.2f post=%.2f",
             (t1 - t0) / 1000.0f,
             (t2 - t1) / 1000.0f,
             (t3 - t2) / 1000.0f);

    return result;
}
} // namespace uhd_detect
