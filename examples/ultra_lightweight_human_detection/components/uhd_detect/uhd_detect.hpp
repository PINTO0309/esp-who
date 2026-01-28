#pragma once

#include "dl_detect_base.hpp"

#include <cstddef>

namespace uhd_detect {
class UltraLightweightHumanDetect : public dl::detect::DetectImpl {
public:
    static inline constexpr float default_score_thr = 0.8f;
    static inline constexpr float default_nms_thr = 0.45f;
    static inline constexpr int default_top_k = 100;

    UltraLightweightHumanDetect(float score_thr = default_score_thr,
                                float nms_thr = default_nms_thr,
                                int top_k = default_top_k);
    ~UltraLightweightHumanDetect() override;
    std::list<dl::detect::result_t> &run(const dl::image::img_t &img) override;

private:
    const uint8_t *m_model_data = nullptr;
    bool m_model_owned = false;
};
} // namespace uhd_detect
