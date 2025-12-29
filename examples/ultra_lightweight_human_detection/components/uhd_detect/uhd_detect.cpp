#include "uhd_detect.hpp"
#include "uhd_constants.hpp"

#include "dl_image_define.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace {
static const char *TAG = "UHDDetect";

constexpr int kInputWidth = 64;
constexpr int kInputHeight = 64;
constexpr int kInputChannels = 3;
constexpr int kInputExp = -7;
constexpr int kTopKDefault = 100;
constexpr float kScoreThrDefault = 0.05f;
constexpr float kNmsThrDefault = 0.45f;
constexpr const char *kOutputBoxName = "box";
constexpr const char *kOutputQualityName = "quality";
constexpr const char *kOutputObjName = "obj";
constexpr const char *kOutputClsName = "cls";

enum class OutputLayout {
    NCHW,
    NHWC,
};

#if defined(UHD_MODEL_W16)
extern const uint8_t ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start;
constexpr const uint8_t *kModelEnd =
    ultratinyod_res_anc8_w16_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end;
#elif defined(UHD_MODEL_W24)
extern const uint8_t ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start;
constexpr const uint8_t *kModelEnd =
    ultratinyod_res_anc8_w24_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end;
#elif defined(UHD_MODEL_W32)
extern const uint8_t ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start;
constexpr const uint8_t *kModelEnd =
    ultratinyod_res_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end;
#elif defined(UHD_MODEL_W40)
extern const uint8_t ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start;
constexpr const uint8_t *kModelEnd =
    ultratinyod_res_anc8_w40_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end;
#elif defined(UHD_MODEL_W48)
extern const uint8_t ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start;
constexpr const uint8_t *kModelEnd =
    ultratinyod_res_anc8_w48_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end;
#elif defined(UHD_MODEL_W56)
extern const uint8_t ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start;
constexpr const uint8_t *kModelEnd =
    ultratinyod_res_anc8_w56_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end;
#else
extern const uint8_t ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start[]
    asm("_binary_ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start");
extern const uint8_t ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end[]
    asm("_binary_ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end");
constexpr const uint8_t *kModelStart =
    ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_start;
constexpr const uint8_t *kModelEnd =
    ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_nocat_espdl_end;
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

float get_tensor_value(const dl::TensorBase &tensor, int idx)
{
    float scale = 1.0f;
    if (tensor.dtype == dl::DATA_TYPE_INT8 || tensor.dtype == dl::DATA_TYPE_INT16) {
        scale = std::ldexp(1.0f, tensor.exponent);
    }

    if (tensor.dtype == dl::DATA_TYPE_INT8) {
        const int8_t *data = static_cast<const int8_t *>(tensor.data);
        return dl::dequantize<int8_t, float>(data[idx], scale);
    }
    if (tensor.dtype == dl::DATA_TYPE_INT16) {
        const int16_t *data = static_cast<const int16_t *>(tensor.data);
        return dl::dequantize<int16_t, float>(data[idx], scale);
    }
    if (tensor.dtype == dl::DATA_TYPE_FLOAT) {
        const float *data = static_cast<const float *>(tensor.data);
        return data[idx];
    }
    return 0.0f;
}

float get_tensor_value(const dl::TensorBase &tensor, OutputLayout layout, int channel, int y, int x)
{
    if (tensor.shape.size() != 4) {
        return 0.0f;
    }

    int h = 0;
    int w = 0;
    int c = 0;
    if (layout == OutputLayout::NCHW) {
        c = tensor.shape[1];
        h = tensor.shape[2];
        w = tensor.shape[3];
        int idx = (channel * h + y) * w + x;
        return get_tensor_value(tensor, idx);
    }

    h = tensor.shape[1];
    w = tensor.shape[2];
    c = tensor.shape[3];
    int idx = (y * w + x) * c + channel;
    return get_tensor_value(tensor, idx);
}

} // namespace

namespace who {
namespace uhd {

UHDDetect::UHDDetect(InputMode mode) :
    m_input_mode(mode),
    m_model(nullptr),
    m_model_data(nullptr),
    m_model_size(0),
    m_model_data_owned(false),
    m_rgb888(kInputWidth * kInputHeight * kInputChannels),
    m_input(kInputWidth * kInputHeight * kInputChannels),
    m_score_thr(kScoreThrDefault),
    m_nms_thr(kNmsThrDefault),
    m_top_k(kTopKDefault)
{
    m_model_size = static_cast<size_t>(kModelEnd - kModelStart);
    m_model_data = kModelStart;
    if (m_model_size == 0) {
        ESP_LOGE(TAG, "model size is zero");
        return;
    }

    uintptr_t model_addr = reinterpret_cast<uintptr_t>(kModelStart);
    if ((model_addr & 0x0F) != 0) {
        uint8_t *aligned = static_cast<uint8_t *>(
            heap_caps_aligned_alloc(16, m_model_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!aligned) {
            aligned = static_cast<uint8_t *>(heap_caps_aligned_alloc(16, m_model_size, MALLOC_CAP_8BIT));
        }
        if (!aligned) {
            ESP_LOGE(TAG, "failed to allocate aligned model buffer (%zu bytes)", m_model_size);
            return;
        }
        std::memcpy(aligned, kModelStart, m_model_size);
        m_model_data = aligned;
        m_model_data_owned = true;
        ESP_LOGW(TAG, "model data copied to aligned buffer");
    }

    bool param_copy = m_model_data_owned;
    m_model = new dl::Model(reinterpret_cast<const char *>(m_model_data),
                            fbs::MODEL_LOCATION_IN_FLASH_RODATA,
                            0,
                            dl::MEMORY_MANAGER_GREEDY,
                            nullptr,
                            param_copy);
    if (!m_model->get_input()) {
        ESP_LOGE(TAG, "model input is nullptr");
    }
}

UHDDetect::~UHDDetect()
{
    delete m_model;
    if (m_model_data_owned && m_model_data) {
        heap_caps_free(const_cast<uint8_t *>(m_model_data));
    }
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

    dl::TensorBase *box = m_model->get_output(kOutputBoxName);
    dl::TensorBase *quality = m_model->get_output(kOutputQualityName);
    dl::TensorBase *obj = m_model->get_output(kOutputObjName);
    dl::TensorBase *cls = m_model->get_output(kOutputClsName);

    if (!box || !quality || !obj || !cls) {
        ESP_LOGE(TAG, "model outputs are nullptr");
        return m_results;
    }
    decode_output_split(*box, *quality, *obj, *cls, img.width, img.height);
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

void UHDDetect::decode_output_split(const dl::TensorBase &box,
                                    const dl::TensorBase &quality,
                                    const dl::TensorBase &obj,
                                    const dl::TensorBase &cls,
                                    int img_w,
                                    int img_h)
{
    if (box.shape.size() != 4 || quality.shape.size() != 4 || obj.shape.size() != 4 || cls.shape.size() != 4) {
        ESP_LOGE(TAG, "unexpected output dims");
        return;
    }

    OutputLayout layout = OutputLayout::NCHW;
    int h = 0;
    int w = 0;
    int box_channels = 0;
    if (box.shape[1] == constants::kNumAnchors * 4) {
        layout = OutputLayout::NCHW;
        h = box.shape[2];
        w = box.shape[3];
        box_channels = box.shape[1];
    } else if (box.shape[3] == constants::kNumAnchors * 4) {
        layout = OutputLayout::NHWC;
        h = box.shape[1];
        w = box.shape[2];
        box_channels = box.shape[3];
    } else {
        ESP_LOGE(TAG, "invalid box output shape");
        return;
    }

    int obj_h = (layout == OutputLayout::NCHW) ? obj.shape[2] : obj.shape[1];
    int obj_w = (layout == OutputLayout::NCHW) ? obj.shape[3] : obj.shape[2];
    int obj_c = (layout == OutputLayout::NCHW) ? obj.shape[1] : obj.shape[3];
    int quality_h = (layout == OutputLayout::NCHW) ? quality.shape[2] : quality.shape[1];
    int quality_w = (layout == OutputLayout::NCHW) ? quality.shape[3] : quality.shape[2];
    int quality_c = (layout == OutputLayout::NCHW) ? quality.shape[1] : quality.shape[3];
    int cls_h = (layout == OutputLayout::NCHW) ? cls.shape[2] : cls.shape[1];
    int cls_w = (layout == OutputLayout::NCHW) ? cls.shape[3] : cls.shape[2];
    int cls_c = (layout == OutputLayout::NCHW) ? cls.shape[1] : cls.shape[3];

    if (h <= 0 || w <= 0 || obj_h != h || obj_w != w || quality_h != h || quality_w != w || cls_h != h ||
        cls_w != w) {
        ESP_LOGE(TAG, "output shape mismatch");
        return;
    }

    const int num_anchors = constants::kNumAnchors;
    if (box_channels % num_anchors != 0 || obj_c % num_anchors != 0 || quality_c % num_anchors != 0 ||
        cls_c % num_anchors != 0) {
        ESP_LOGE(TAG, "channel/anchor mismatch");
        return;
    }

    const int box_per_anchor = box_channels / num_anchors;
    if (box_per_anchor < 4) {
        ESP_LOGE(TAG, "invalid box channel count");
        return;
    }

    const int obj_per_anchor = obj_c / num_anchors;
    const int quality_per_anchor = quality_c / num_anchors;
    const int cls_per_anchor = cls_c / num_anchors;

    static int log_counter = 0;
    std::vector<CandidateBox> boxes;
    boxes.reserve(num_anchors * h * w);
    float frame_max_score = 0.0f;

    for (int a = 0; a < num_anchors; a++) {
        float anchor_w = constants::kAnchors[a][0] * constants::kWhScale[a][0];
        float anchor_h = constants::kAnchors[a][1] * constants::kWhScale[a][1];
        int box_base = a * box_per_anchor;
        int obj_base = a * obj_per_anchor;
        int quality_base = a * quality_per_anchor;
        int cls_base = a * cls_per_anchor;

        for (int gy = 0; gy < h; gy++) {
            for (int gx = 0; gx < w; gx++) {
                float tx = get_tensor_value(box, layout, box_base + 0, gy, gx);
                float ty = get_tensor_value(box, layout, box_base + 1, gy, gx);
                float tw = get_tensor_value(box, layout, box_base + 2, gy, gx);
                float th = get_tensor_value(box, layout, box_base + 3, gy, gx);

                float obj_score = obj_per_anchor > 0 ? sigmoid(get_tensor_value(obj, layout, obj_base, gy, gx)) : 1.0f;
                float quality_score =
                    quality_per_anchor > 0 ? sigmoid(get_tensor_value(quality, layout, quality_base, gy, gx)) : 1.0f;

                float best_score = 0.0f;
                int best_cls = 0;
                if (cls_per_anchor > 0) {
                    for (int cls_idx = 0; cls_idx < cls_per_anchor; cls_idx++) {
                        float cls_score = sigmoid(get_tensor_value(cls, layout, cls_base + cls_idx, gy, gx));
                        float score = obj_score * quality_score * cls_score;
                        if (score > best_score) {
                            best_score = score;
                            best_cls = cls_idx;
                        }
                    }
                } else {
                    best_score = obj_score * quality_score;
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

    for (const auto &box_item : boxes) {
        dl::detect::result_t res;
        res.category = box_item.category;
        res.score = box_item.score;
        res.box = {
            static_cast<int>(std::lround(box_item.x1)),
            static_cast<int>(std::lround(box_item.y1)),
            static_cast<int>(std::lround(box_item.x2)),
            static_cast<int>(std::lround(box_item.y2)),
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
