#include "uhd_detect.hpp"
#include "uhd_constants.hpp"

#include "dl_image_define.hpp"
#include "esp_log.h"
#include "esp_timer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
static const char *TAG = "UHDDetect";

constexpr int kInputWidth = 64;
constexpr int kInputHeight = 64;
constexpr int kInputChannels = 3;
constexpr int kInputExp = -7;
constexpr int kTopKDefault = 100;
constexpr float kScoreThrDefault = 0.05f;
constexpr float kNmsThrDefault = 0.45f;
constexpr bool kHasQuality = true;
constexpr const char *kOutputName = "txtywh_obj_quality_cls_x8";

#if defined(UHD_MODEL_W16)
extern const uint8_t ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_espdl_start;
constexpr const uint8_t *kModelEnd = ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_espdl_end;
#elif defined(UHD_MODEL_W24)
extern const uint8_t ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_espdl_start;
constexpr const uint8_t *kModelEnd = ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_espdl_end;
#elif defined(UHD_MODEL_W32)
extern const uint8_t ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_espdl_start;
constexpr const uint8_t *kModelEnd = ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_espdl_end;
#elif defined(UHD_MODEL_W40)
extern const uint8_t ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_espdl_start;
constexpr const uint8_t *kModelEnd = ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_espdl_end;
#elif defined(UHD_MODEL_W48)
extern const uint8_t ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_espdl_start;
constexpr const uint8_t *kModelEnd = ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_espdl_end;
#elif defined(UHD_MODEL_W56)
extern const uint8_t ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_espdl_start;
constexpr const uint8_t *kModelEnd = ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_espdl_end;
#else
extern const uint8_t ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_espdl_start;
constexpr const uint8_t *kModelEnd = ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_espdl_end;
#endif

float sigmoid(float x)
{
    return 1.0f / (1.0f + std::exp(-x));
}

float softplus(float x)
{
    if (x > 20.0f) {
        return x;
    }
    if (x < -20.0f) {
        return std::exp(x);
    }
    return std::log1p(std::exp(x));
}

float clamp01(float x)
{
    if (x < 0.0f) {
        return 0.0f;
    }
    if (x > 1.0f) {
        return 1.0f;
    }
    return x;
}

} // namespace

namespace who {
namespace uhd {

UHDDetect::UHDDetect(InputMode mode) :
    m_input_mode(mode),
    m_model(nullptr),
    m_rgb888(kInputWidth * kInputHeight * kInputChannels),
    m_input(kInputWidth * kInputHeight * kInputChannels),
    m_score_thr(kScoreThrDefault),
    m_nms_thr(kNmsThrDefault),
    m_top_k(kTopKDefault)
{
    (void)kModelEnd;
    m_model = new dl::Model(reinterpret_cast<const char *>(kModelStart),
                            fbs::MODEL_LOCATION_IN_FLASH_RODATA,
                            0,
                            dl::MEMORY_MANAGER_GREEDY,
                            nullptr,
                            false);
    if (!m_model->get_input()) {
        ESP_LOGE(TAG, "model input is nullptr");
    }
}

UHDDetect::~UHDDetect()
{
    delete m_model;
}

std::list<dl::detect::result_t> &UHDDetect::run(const dl::image::img_t &img)
{
    m_results.clear();
    int64_t t_start = esp_timer_get_time();
    if (!prepare_input(img)) {
        return m_results;
    }
    int64_t t_pre = esp_timer_get_time();

    dl::TensorBase input_tensor({1, kInputHeight, kInputWidth, kInputChannels},
                                m_input.data(),
                                kInputExp,
                                dl::DATA_TYPE_INT8,
                                false);

    m_model->run(&input_tensor);
    int64_t t_infer = esp_timer_get_time();

    dl::TensorBase *output = m_model->get_output(kOutputName);
    if (!output) {
        output = m_model->get_output();
    }
    if (!output) {
        ESP_LOGE(TAG, "model output is nullptr");
        return m_results;
    }

    decode_output(*output, img.width, img.height);
    int64_t t_post = esp_timer_get_time();

    static int log_counter = 0;
    if ((log_counter++ % 30) == 0) {
        float pre_ms = (t_pre - t_start) / 1000.0f;
        float infer_ms = (t_infer - t_pre) / 1000.0f;
        float post_ms = (t_post - t_infer) / 1000.0f;
        ESP_LOGI(TAG, "timing: pre=%.2fms infer=%.2fms post=%.2fms", pre_ms, infer_ms, post_ms);
    }
    return m_results;
}

dl::detect::Detect &UHDDetect::set_score_thr(float score_thr, int idx)
{
    (void)idx;
    m_score_thr = score_thr;
    return *this;
}

dl::detect::Detect &UHDDetect::set_nms_thr(float nms_thr, int idx)
{
    (void)idx;
    m_nms_thr = nms_thr;
    return *this;
}

dl::Model *UHDDetect::get_raw_model(int idx)
{
    (void)idx;
    return m_model;
}

bool UHDDetect::prepare_input(const dl::image::img_t &img)
{
    dl::image::img_t dst_img = {
        .data = m_rgb888.data(),
        .width = static_cast<uint16_t>(kInputWidth),
        .height = static_cast<uint16_t>(kInputHeight),
        .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888,
    };

    uint32_t caps = 0;
    if (img.pix_type == dl::image::DL_IMAGE_PIX_TYPE_RGB565) {
        caps = dl::image::DL_IMAGE_CAP_RGB565_BIG_ENDIAN;
    }

    m_transformer.reset();
    m_transformer.set_src_img(img).set_dst_img(dst_img).set_caps(caps);
    if (m_transformer.transform() != ESP_OK) {
        ESP_LOGE(TAG, "image transform failed");
        return false;
    }

    const float inv_scale = std::ldexp(1.0f, -kInputExp);
    const float inv_255 = 1.0f / 255.0f;

    for (size_t i = 0; i < m_rgb888.size(); i += 3) {
        float r = m_rgb888[i] * inv_255;
        float g = m_rgb888[i + 1] * inv_255;
        float b = m_rgb888[i + 2] * inv_255;

        float c1 = 0.0f;
        float c2 = 0.0f;
        float c3 = 0.0f;

        if (m_input_mode == InputMode::RGB888) {
            c1 = r;
            c2 = g;
            c3 = b;
        } else {
            float y = clamp01(0.299f * r + 0.587f * g + 0.114f * b);
            if (m_input_mode == InputMode::YUV422) {
                float u = -0.14713f * r - 0.28886f * g + 0.436f * b;
                float v = 0.615f * r - 0.51499f * g - 0.10001f * b;
                c1 = y;
                c2 = clamp01(u + 0.5f);
                c3 = clamp01(v + 0.5f);
            } else if (m_input_mode == InputMode::Y_TERNARY) {
                float yq = (y < (1.0f / 3.0f)) ? 0.0f : (y < (2.0f / 3.0f) ? 0.5f : 1.0f);
                c1 = yq;
                c2 = yq;
                c3 = yq;
            } else if (m_input_mode == InputMode::Y_BINARY) {
                float yq = (y >= 0.5f) ? 1.0f : 0.0f;
                c1 = yq;
                c2 = yq;
                c3 = yq;
            } else {
                c1 = y;
                c2 = y;
                c3 = y;
            }
        }

        m_input[i] = dl::quantize<int8_t>(c1, inv_scale);
        m_input[i + 1] = dl::quantize<int8_t>(c2, inv_scale);
        m_input[i + 2] = dl::quantize<int8_t>(c3, inv_scale);
    }

    return true;
}

void UHDDetect::decode_output(const dl::TensorBase &output, int img_w, int img_h)
{
    if (output.shape.size() != 4) {
        ESP_LOGE(TAG, "unexpected output dims");
        return;
    }

    const int b = output.shape[0];
    const int c = output.shape[1];
    const int h = output.shape[2];
    const int w = output.shape[3];

    if (b != 1 || h <= 0 || w <= 0 || c <= 0) {
        ESP_LOGE(TAG, "invalid output shape");
        return;
    }

    const int num_anchors = constants::kNumAnchors;
    if (c % num_anchors != 0) {
        ESP_LOGE(TAG, "channel/anchor mismatch: c=%d anchors=%d", c, num_anchors);
        return;
    }

    const int per_anchor = c / num_anchors;
    const int quality_extra = (kHasQuality && per_anchor >= 6) ? 1 : 0;
    const int num_classes = per_anchor - 5 - quality_extra;
    if (num_classes <= 0) {
        ESP_LOGE(TAG, "invalid class count: %d", num_classes);
        return;
    }

    float scale = 1.0f;
    if (output.dtype == dl::DATA_TYPE_INT8 || output.dtype == dl::DATA_TYPE_INT16) {
        scale = std::ldexp(1.0f, output.exponent);
    }

    auto get_val = [&](int channel, int y, int x) -> float {
        int idx = (channel * h + y) * w + x;
        if (output.dtype == dl::DATA_TYPE_INT8) {
            const int8_t *data = static_cast<const int8_t *>(output.data);
            return dl::dequantize<int8_t, float>(data[idx], scale);
        }
        if (output.dtype == dl::DATA_TYPE_INT16) {
            const int16_t *data = static_cast<const int16_t *>(output.data);
            return dl::dequantize<int16_t, float>(data[idx], scale);
        }
        if (output.dtype == dl::DATA_TYPE_FLOAT) {
            const float *data = static_cast<const float *>(output.data);
            return data[idx];
        }
        return 0.0f;
    };

    static int log_counter = 0;
    std::vector<CandidateBox> boxes;
    boxes.reserve(num_anchors * h * w);
    float frame_max_score = 0.0f;

    for (int a = 0; a < num_anchors; a++) {
        float anchor_w = constants::kAnchors[a][0] * constants::kWhScale[a][0];
        float anchor_h = constants::kAnchors[a][1] * constants::kWhScale[a][1];
        int base = a * per_anchor;

        for (int gy = 0; gy < h; gy++) {
            for (int gx = 0; gx < w; gx++) {
                float tx = get_val(base + 0, gy, gx);
                float ty = get_val(base + 1, gy, gx);
                float tw = get_val(base + 2, gy, gx);
                float th = get_val(base + 3, gy, gx);
                float obj = sigmoid(get_val(base + 4, gy, gx));
                float quality = 1.0f;
                if (quality_extra) {
                    quality = sigmoid(get_val(base + 5, gy, gx));
                }

                float best_score = 0.0f;
                int best_cls = 0;
                for (int cls = 0; cls < num_classes; cls++) {
                    float cls_score = sigmoid(get_val(base + 5 + quality_extra + cls, gy, gx));
                    float score = obj * quality * cls_score;
                    if (score > best_score) {
                        best_score = score;
                        best_cls = cls;
                    }
                }

                frame_max_score = std::max(frame_max_score, best_score);
                if (best_score < m_score_thr) {
                    continue;
                }

                float cx = (sigmoid(tx) + static_cast<float>(gx)) / static_cast<float>(w);
                float cy = (sigmoid(ty) + static_cast<float>(gy)) / static_cast<float>(h);
                float bw = anchor_w * softplus(tw);
                float bh = anchor_h * softplus(th);

                float x1 = (cx - bw * 0.5f) * img_w;
                float y1 = (cy - bh * 0.5f) * img_h;
                float x2 = (cx + bw * 0.5f) * img_w;
                float y2 = (cy + bh * 0.5f) * img_h;

                boxes.push_back({best_cls, best_score, x1, y1, x2, y2});
            }
        }
    }

    if (boxes.empty()) {
        if ((log_counter++ % 30) == 0) {
            ESP_LOGI(TAG, "detections: 0 (max_score=%.4f)", frame_max_score);
        }
        return;
    }

    std::sort(boxes.begin(), boxes.end(),
              [](const CandidateBox &a, const CandidateBox &b) { return a.score > b.score; });

    if (static_cast<int>(boxes.size()) > m_top_k) {
        boxes.resize(m_top_k);
    }

    apply_nms(boxes);

    if ((log_counter++ % 30) == 0) {
        ESP_LOGI(TAG, "detections: %zu (max_score=%.4f)", boxes.size(), frame_max_score);
    }

    for (const auto &box : boxes) {
        dl::detect::result_t res;
        res.category = box.category;
        res.score = box.score;
        res.box = {
            static_cast<int>(std::lround(box.x1)),
            static_cast<int>(std::lround(box.y1)),
            static_cast<int>(std::lround(box.x2)),
            static_cast<int>(std::lround(box.y2)),
        };
        res.limit_box(img_w, img_h);
        m_results.push_back(res);
    }
}

void UHDDetect::apply_nms(std::vector<CandidateBox> &boxes)
{
    std::vector<CandidateBox> kept;
    kept.reserve(boxes.size());
    std::vector<bool> suppressed(boxes.size(), false);

    for (size_t i = 0; i < boxes.size(); i++) {
        if (suppressed[i]) {
            continue;
        }
        kept.push_back(boxes[i]);
        if (static_cast<int>(kept.size()) >= m_top_k) {
            break;
        }

        const auto &a = boxes[i];
        float area_a = std::max(0.0f, a.x2 - a.x1 + 1.0f) * std::max(0.0f, a.y2 - a.y1 + 1.0f);
        for (size_t j = i + 1; j < boxes.size(); j++) {
            if (suppressed[j]) {
                continue;
            }
            const auto &b = boxes[j];
            float inter_x1 = std::max(a.x1, b.x1);
            float inter_y1 = std::max(a.y1, b.y1);
            float inter_x2 = std::min(a.x2, b.x2);
            float inter_y2 = std::min(a.y2, b.y2);
            float inter_w = inter_x2 - inter_x1 + 1.0f;
            float inter_h = inter_y2 - inter_y1 + 1.0f;
            if (inter_w <= 0.0f || inter_h <= 0.0f) {
                continue;
            }
            float inter_area = inter_w * inter_h;
            float area_b = std::max(0.0f, b.x2 - b.x1 + 1.0f) * std::max(0.0f, b.y2 - b.y1 + 1.0f);
            float iou = inter_area / (area_a + area_b - inter_area);
            if (iou > m_nms_thr) {
                suppressed[j] = true;
            }
        }
    }

    boxes.swap(kept);
}

} // namespace uhd
} // namespace who
