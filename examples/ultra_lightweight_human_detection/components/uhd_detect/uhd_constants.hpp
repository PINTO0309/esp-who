#pragma once

#include <cstddef>
#include <cstring>

namespace uhd_detect {
struct UhdAnchorSet {
    const float *anchors;
    const float *wh_scale;
    int count;
};

static inline constexpr float kAnchorsW32[] = {
    2.1544415e-06f, 4.8916531e-06f,
    4.3464302e-06f, 7.8658741e-06f,
    5.0172557e-06f, 1.5973144e-05f,
    1.0519245e-05f, 1.4973825e-05f,
    9.8587534e-06f, 3.3517914e-05f,
    1.8386805e-05f, 5.0171810e-05f,
    3.2672662e-05f, 7.2077630e-05f,
    6.5735781e-05f, 8.7482200e-05f,
};

static inline constexpr float kWhScaleW32[] = {
    1.1018596f, 1.1018785f,
    1.1399714f, 1.1158636f,
    1.1602806f, 1.1600958f,
    1.1953557f, 1.1898705f,
    1.0877644f, 1.0939205f,
    1.1106393f, 1.2792205f,
    1.2914679f, 1.2122172f,
    14.072575f, 16.930525f,
};

static inline constexpr float kAnchorsW40[] = {
    2.1553853e-06f, 4.8937904e-06f,
    4.3483524e-06f, 7.8692810e-06f,
    5.0194476e-06f, 1.5980148e-05f,
    1.0523835e-05f, 1.4980503e-05f,
    9.8630517e-06f, 3.3532524e-05f,
    1.8394810e-05f, 5.0193834e-05f,
    3.2686890e-05f, 7.2109266e-05f,
    6.5764492e-05f, 8.7520377e-05f,
};

static inline constexpr float kWhScaleW40[] = {
    1.1052004f, 0.7689974f,
    1.1248873f, 1.1524976f,
    1.1474544f, 1.1747091f,
    1.1839403f, 1.1782105f,
    1.1877522f, 1.2349045f,
    0.9627373f, 1.1769345f,
    1.1073434f, 1.1735326f,
    13.743571f, 16.26899f,
};

inline bool get_uhd_anchor_set(const char *model_name, UhdAnchorSet *out)
{
    if (!model_name || !out) {
        return false;
    }
    if (std::strstr(model_name, "w40") != nullptr) {
        out->anchors = kAnchorsW40;
        out->wh_scale = kWhScaleW40;
        out->count = 8;
        return true;
    }
    if (std::strstr(model_name, "w32") != nullptr) {
        out->anchors = kAnchorsW32;
        out->wh_scale = kWhScaleW32;
        out->count = 8;
        return true;
    }
    return false;
}
} // namespace uhd_detect
