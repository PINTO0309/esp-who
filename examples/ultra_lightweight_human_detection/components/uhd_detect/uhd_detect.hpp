#pragma once

#include "dl_detect_base.hpp"
#include "dl_image.hpp"
#include "dl_model_base.hpp"
#include "dl_tensor_base.hpp"

#include <cstddef>
#include <list>
#include <string>
#include <vector>

namespace who {
namespace uhd {

enum class InputMode {
    RGB888,
    YUV422,
    Y_ONLY,
    Y_TERNARY,
    Y_BINARY,
};

class UHDDetect : public dl::detect::Detect {
public:
    explicit UHDDetect(InputMode mode = InputMode::RGB888);
    ~UHDDetect() override;

    std::list<dl::detect::result_t> &run(const dl::image::img_t &img) override;
    dl::detect::Detect &set_score_thr(float score_thr, int idx = 0) override;
    dl::detect::Detect &set_nms_thr(float nms_thr, int idx = 0) override;
    dl::Model *get_raw_model(int idx = 0) override;

    void set_input_mode(InputMode mode) { m_input_mode = mode; }

private:
    struct CandidateBox {
        int category;
        float score;
        float x1;
        float y1;
        float x2;
        float y2;
    };

    bool prepare_input(const dl::image::img_t &img);
    void decode_output_split(const dl::TensorBase &box,
                             const dl::TensorBase &quality,
                             const dl::TensorBase &obj,
                             const dl::TensorBase &cls,
                             int img_w,
                             int img_h);
    void apply_nms(std::vector<CandidateBox> &boxes);

    InputMode m_input_mode;
    dl::Model *m_model;
    const uint8_t *m_model_data;
    size_t m_model_size;
    bool m_model_data_owned;
    dl::image::ImageTransformer m_transformer;
    std::vector<uint8_t> m_rgb888;
    std::vector<int8_t> m_input;
    std::list<dl::detect::result_t> m_results;
    float m_score_thr;
    float m_nms_thr;
    int m_top_k;
};

} // namespace uhd
} // namespace who
