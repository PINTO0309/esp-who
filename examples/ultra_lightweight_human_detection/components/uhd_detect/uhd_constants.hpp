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

static inline constexpr float kAnchorsAnc16W32[] = {
    3.02635658e-06f, 6.46879380e-06f, 6.32300817e-06f, 1.40926313e-05f,
    8.13052793e-06f, 1.96054280e-05f, 9.71134068e-06f, 2.68001568e-05f,
    1.17439813e-05f, 3.32903182e-05f, 1.40024895e-05f, 3.99599194e-05f,
    1.67134622e-05f, 4.57733149e-05f, 1.92088009e-05f, 5.26965669e-05f,
    2.19081558e-05f, 6.07231996e-05f, 2.55193081e-05f, 6.68508656e-05f,
    2.98108807e-05f, 7.16103677e-05f, 3.45543995e-05f, 7.66658413e-05f,
    4.14421411e-05f, 8.07022880e-05f, 4.99125345e-05f, 8.63856039e-05f,
    6.13213706e-05f, 9.16725039e-05f, 8.26347023e-05f, 9.93725262e-05f,
};

static inline constexpr float kWhScaleAnc16W32[] = {
    1.13001645e+00f, 1.17272377e+00f, 1.14395380e+00f, 1.17324901e+00f,
    1.19282222e+00f, 1.23107517e+00f, 1.13110387e+00f, 1.23999238e+00f,
    1.23372805e+00f, 1.20248353e+00f, 1.21209586e+00f, 1.20939171e+00f,
    1.07584357e+00f, 1.15044999e+00f, 1.13603473e+00f, 1.21550882e+00f,
    1.26873398e+00f, 1.18509626e+00f, 1.06293714e+00f, 1.28947246e+00f,
    1.13906920e+00f, 1.30407476e+00f, 1.27542281e+00f, 1.19643128e+00f,
    9.22454536e-01f, 1.31013215e+00f, 1.20553207e+00f, 1.37177920e+00f,
    1.14782047e+00f, 1.22924173e+00f, 1.34597931e+01f, 1.57487030e+01f,
};

inline bool get_uhd_anchor_set(const char *model_name, UhdAnchorSet *out)
{
    if (!model_name || !out) {
        return false;
    }
    if (std::strstr(model_name, "anc16") != nullptr) {
        out->anchors = kAnchorsAnc16W32;
        out->wh_scale = kWhScaleAnc16W32;
        out->count = 16;
        return true;
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
