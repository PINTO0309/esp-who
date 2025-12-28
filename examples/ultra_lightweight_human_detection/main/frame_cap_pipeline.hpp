#pragma once
#include "who_frame_cap.hpp"

#if CONFIG_IDF_TARGET_ESP32S3
who::frame_cap::WhoFrameCap *get_frame_cap_pipeline();
#endif
