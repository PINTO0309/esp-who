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
    4.7440167e-06f, 1.0240427e-05f,
    1.5362340e-05f, 4.2770040e-05f,
    2.1009639e-05f, 5.8622478e-05f,
    2.7561418e-05f, 6.8975314e-05f,
    3.4787725e-05f, 7.6808254e-05f,
    4.4954682e-05f, 8.2734783e-05f,
    5.7605772e-05f, 9.0361682e-05f,
    8.0174403e-05f, 9.9399280e-05f,
};

static inline constexpr float kWhScaleW32[] = {
    1.2035773f, 1.2207098f,
    1.1080858f, 1.2721308f,
    1.2660776f, 1.2729119f,
    1.0586028f, 1.3165711f,
    1.2519584f, 1.1171706f,
    1.1899507f, 1.3121780f,
    1.2618430f, 1.3428993f,
    14.004194f, 15.676925f,
};

static inline constexpr float kAnchorsAnc8W32Head[] = {
    2.7083000e-02f, 4.1668091e-02f,
    6.8121016e-02f, 1.0279302e-01f,
    8.7500028e-02f, 1.3055627e-01f,
    1.0625814e-01f, 1.5750004e-01f,
    1.3125019e-01f, 1.9166660e-01f,
    1.6458280e-01f, 2.3333186e-01f,
    2.1249996e-01f, 3.0000007e-01f,
    3.1042215e-01f, 4.3888870e-01f,
};

static inline constexpr float kWhScaleAnc8W32Head[] = {
    2.9691894e+00f, 1.2218597e+00f,
    1.0289718e+00f, 1.0665035e+00f,
    1.0475363e+00f, 1.1985750e+00f,
    1.3626738e+00f, 1.1803708e+00f,
    1.2696019e+00f, 1.3440658e+00f,
    1.1940233e+00f, 1.3933610e+00f,
    1.1819617e+00f, 1.3163528e+00f,
    9.6330529e-01f, 8.6391145e-01f,
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
    if (std::strstr(model_name, "nopost_head") != nullptr) {
        out->anchors = kAnchorsAnc8W32Head;
        out->wh_scale = kWhScaleAnc8W32Head;
        out->count = 8;
        return true;
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
