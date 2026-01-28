#include "frame_cap_pipeline.hpp"
#include "who_cam.hpp"

using namespace who::cam;
using namespace who::frame_cap;

// num of frames the model takes to get result
#define MODEL_TIME 2
#define MODEL_INPUT_W 64
#define MODEL_INPUT_H 64

// The size of the fb_count and ringbuf_len must be big enough. If you have no idea how to set them, try with 5 and
// larger.
#if CONFIG_IDF_TARGET_ESP32S3
WhoFrameCap *get_lcd_dvp_frame_cap_pipeline()
{
    framesize_t frame_size = FRAMESIZE_96X96;
#ifdef BSP_BOARD_ESP32_S3_KORVO_2
    auto cam = new WhoS3Cam(PIXFORMAT_RGB565, frame_size, MODEL_TIME + 3, true, true);
#else
    auto cam = new WhoS3Cam(PIXFORMAT_RGB565, frame_size, MODEL_TIME + 3);
#endif
    auto frame_cap = new WhoFrameCap();
    frame_cap->add_node<WhoFetchNode>("FrameCapFetch", cam);
    return frame_cap;
}

WhoFrameCap *get_term_dvp_frame_cap_pipeline()
{
    framesize_t frame_size = FRAMESIZE_96X96;
#ifdef BSP_BOARD_ESP32_S3_KORVO_2
    auto cam = new WhoS3Cam(PIXFORMAT_RGB565, frame_size, MODEL_TIME + 2, true, true);
#else
    auto cam = new WhoS3Cam(PIXFORMAT_RGB565, frame_size, MODEL_TIME + 2);
#endif
    auto frame_cap = new WhoFrameCap();
    frame_cap->add_node<WhoFetchNode>("FrameCapFetch", cam);
    return frame_cap;
}
#elif CONFIG_IDF_TARGET_ESP32P4
WhoFrameCap *get_lcd_mipi_csi_frame_cap_pipeline()
{
    auto cam = new WhoP4Cam(V4L2_PIX_FMT_RGB565, MODEL_TIME + 3);
    auto frame_cap = new WhoFrameCap();
    frame_cap->add_node<WhoFetchNode>("FrameCapFetch", cam);
    return frame_cap;
}

WhoFrameCap *get_lcd_mipi_csi_ppa_frame_cap_pipeline(WhoFrameCapNode **lcd_disp_frame_cap_node)
{
    auto cam = new WhoP4Cam(V4L2_PIX_FMT_RGB565, MODEL_TIME + 4);
    auto frame_cap = new WhoFrameCap();
    frame_cap->add_node<WhoFetchNode>("FrameCapFetch", cam);
    frame_cap->add_node<WhoPPAResizeNode>(
        "FrameCapPPAResize", MODEL_INPUT_W, MODEL_INPUT_H, dl::image::DL_IMAGE_PIX_TYPE_RGB565, MODEL_TIME);
    *lcd_disp_frame_cap_node = frame_cap->get_node("FrameCapFetch");
    return frame_cap;
}

WhoFrameCap *get_lcd_uvc_frame_cap_pipeline(WhoFrameCapNode **lcd_disp_frame_cap_node)
{
    auto cam = new WhoUVCCam(UVC_VS_FORMAT_MJPEG, 640, 480, 30, 4);
    auto frame_cap = new WhoFrameCap();
    frame_cap->add_node<WhoFetchNode>("FrameCapFetch", cam, false);
    frame_cap->add_node<WhoDecodeNode>("FrameCapDecode", dl::image::DL_IMAGE_PIX_TYPE_RGB565, 2, false);
    frame_cap->add_node<WhoPPAResizeNode>(
        "FrameCapPPAResize", MODEL_INPUT_W, MODEL_INPUT_H, dl::image::DL_IMAGE_PIX_TYPE_RGB565, MODEL_TIME + 1);
    *lcd_disp_frame_cap_node = frame_cap->get_node("FrameCapDecode");
    return frame_cap;
}

WhoFrameCap *get_term_mipi_csi_frame_cap_pipeline()
{
    auto cam = new WhoP4Cam(V4L2_PIX_FMT_RGB565, MODEL_TIME + 2);
    auto frame_cap = new WhoFrameCap();
    frame_cap->add_node<WhoFetchNode>("FrameCapFetch", cam);
    return frame_cap;
}

WhoFrameCap *get_term_mipi_csi_ppa_frame_cap_pipeline()
{
    auto cam = new WhoP4Cam(V4L2_PIX_FMT_RGB565, MODEL_TIME + 3);
    auto frame_cap = new WhoFrameCap();
    frame_cap->add_node<WhoFetchNode>("FrameCapFetch", cam);
    frame_cap->add_node<WhoPPAResizeNode>(
        "FrameCapPPAResize", MODEL_INPUT_W, MODEL_INPUT_H, dl::image::DL_IMAGE_PIX_TYPE_RGB565, MODEL_TIME);
    return frame_cap;
}

WhoFrameCap *get_term_uvc_frame_cap_pipeline()
{
    auto cam = new WhoUVCCam(UVC_VS_FORMAT_MJPEG, 640, 480, 30, 4);
    auto frame_cap = new WhoFrameCap();
    frame_cap->add_node<WhoFetchNode>("FrameCapFetch", cam, false);
    frame_cap->add_node<WhoDecodeNode>("FrameCapDecode", dl::image::DL_IMAGE_PIX_TYPE_RGB565, MODEL_TIME);
    frame_cap->add_node<WhoPPAResizeNode>(
        "FrameCapPPAResize", MODEL_INPUT_W, MODEL_INPUT_H, dl::image::DL_IMAGE_PIX_TYPE_RGB565, MODEL_TIME);
    return frame_cap;
}
#endif
