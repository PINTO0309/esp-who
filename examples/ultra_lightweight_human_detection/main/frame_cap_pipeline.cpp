#include "frame_cap_pipeline.hpp"
#include "who_cam.hpp"

using namespace who::cam;
using namespace who::frame_cap;

// num of frames the model take to get result
#define MODEL_TIME 2

#if CONFIG_IDF_TARGET_ESP32S3
WhoFrameCap *get_frame_cap_pipeline()
{
    // Keep the camera resolution small to reduce processing cost.
    framesize_t frame_size = FRAMESIZE_96X96;
    auto cam = new WhoS3Cam(PIXFORMAT_RGB565, frame_size, MODEL_TIME + 3);
    auto frame_cap = new WhoFrameCap();
    frame_cap->add_node<WhoFetchNode>("FrameCapFetch", cam);
    return frame_cap;
}
#endif
