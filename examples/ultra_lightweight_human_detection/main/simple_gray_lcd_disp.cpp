#include "simple_gray_lcd_disp.hpp"

#if BSP_CONFIG_NO_GRAPHIC_LIB

#include "who_detect_result_handle.hpp"

#include "dl_image_define.hpp"
#include "esp_log.h"

namespace who {
namespace lcd_disp {

namespace {
constexpr float kYFromR = 0.299f;
constexpr float kYFromG = 0.587f;
constexpr float kYFromB = 0.114f;
} // namespace

SimpleGrayLCDDisp::SimpleGrayLCDDisp(const std::string &name,
                                     frame_cap::WhoFrameCapNode *frame_cap_node,
                                     const std::vector<std::vector<uint8_t>> &palette,
                                     int peek_index) :
    task::WhoTask(name),
    m_lcd(new lcd::WhoLCD()),
    m_frame_cap_node(frame_cap_node),
    m_peek_index(peek_index),
    m_palette565(),
    m_res_mutex(xSemaphoreCreateMutex()),
    m_result()
{
    m_palette565.reserve(palette.size());
    for (const auto &color : palette) {
        uint8_t r = color.size() > 0 ? color[0] : 0;
        uint8_t g = color.size() > 1 ? color[1] : 0;
        uint8_t b = color.size() > 2 ? color[2] : 0;
        uint16_t rgb565 = static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        m_palette565.push_back({static_cast<uint8_t>(rgb565 >> 8), static_cast<uint8_t>(rgb565 & 0xFF)});
    }
    frame_cap_node->add_new_frame_signal_subscriber(this);
}

SimpleGrayLCDDisp::~SimpleGrayLCDDisp()
{
    delete m_lcd;
    vSemaphoreDelete(m_res_mutex);
}

void SimpleGrayLCDDisp::save_detect_result(const detect::WhoDetect::result_t &result)
{
    xSemaphoreTake(m_res_mutex, portMAX_DELAY);
    m_results.push(result);
    xSemaphoreGive(m_res_mutex);
}

void SimpleGrayLCDDisp::cleanup_results()
{
    xSemaphoreTake(m_res_mutex, portMAX_DELAY);
    std::queue<detect::WhoDetect::result_t>().swap(m_results);
    m_result = {};
    xSemaphoreGive(m_res_mutex);
}

void SimpleGrayLCDDisp::ensure_buffer(uint16_t width, uint16_t height)
{
    const size_t needed = static_cast<size_t>(width) * height * 2;
    if (m_gray_buf.size() != needed) {
        m_gray_buf.assign(needed, 0);
    }
}

void SimpleGrayLCDDisp::convert_rgb565_to_gray(const uint8_t *src, uint8_t *dst, int width, int height)
{
    const int pixel_count = width * height;
    for (int i = 0; i < pixel_count; i++) {
        const uint16_t pixel = static_cast<uint16_t>((src[i * 2] << 8) | src[i * 2 + 1]);
        const uint8_t r5 = (pixel >> 11) & 0x1F;
        const uint8_t g6 = (pixel >> 5) & 0x3F;
        const uint8_t b5 = pixel & 0x1F;

        const uint8_t r8 = static_cast<uint8_t>((r5 * 255 + 15) / 31);
        const uint8_t g8 = static_cast<uint8_t>((g6 * 255 + 31) / 63);
        const uint8_t b8 = static_cast<uint8_t>((b5 * 255 + 15) / 31);
        const uint8_t y = static_cast<uint8_t>(kYFromR * r8 + kYFromG * g8 + kYFromB * b8);

        uint16_t gray565 = static_cast<uint16_t>(((y >> 3) << 11) | ((y >> 2) << 5) | (y >> 3));
        dst[i * 2] = static_cast<uint8_t>(gray565 >> 8);
        dst[i * 2 + 1] = static_cast<uint8_t>(gray565 & 0xFF);
    }
}

void SimpleGrayLCDDisp::convert_rgb888_to_gray(const uint8_t *src, uint8_t *dst, int width, int height)
{
    const int pixel_count = width * height;
    for (int i = 0; i < pixel_count; i++) {
        const int base = i * 3;
        const uint8_t r = src[base];
        const uint8_t g = src[base + 1];
        const uint8_t b = src[base + 2];
        const uint8_t y = static_cast<uint8_t>(kYFromR * r + kYFromG * g + kYFromB * b);
        uint16_t gray565 = static_cast<uint16_t>(((y >> 3) << 11) | ((y >> 2) << 5) | (y >> 3));
        dst[i * 2] = static_cast<uint8_t>(gray565 >> 8);
        dst[i * 2 + 1] = static_cast<uint8_t>(gray565 & 0xFF);
    }
}

void SimpleGrayLCDDisp::update_result(const cam::cam_fb_t *fb)
{
    auto compare_timestamp = [](const struct timeval &t1, const struct timeval &t2) -> bool {
        if (t1.tv_sec == t2.tv_sec) {
            return t1.tv_usec < t2.tv_usec;
        }
        return t1.tv_sec < t2.tv_sec;
    };

    xSemaphoreTake(m_res_mutex, portMAX_DELAY);
    struct timeval frame_time = fb->timestamp;
    while (!m_results.empty()) {
        detect::WhoDetect::result_t result = m_results.front();
        if (!compare_timestamp(frame_time, result.timestamp)) {
            m_result = result;
            m_results.pop();
        } else {
            break;
        }
    }
    xSemaphoreGive(m_res_mutex);
}

void SimpleGrayLCDDisp::task()
{
    {
        // Clear the full LCD once so untouched regions stay black.
        const size_t clear_size = static_cast<size_t>(BSP_LCD_H_RES) * BSP_LCD_V_RES * (BSP_LCD_BITS_PER_PIXEL / 8);
        std::vector<uint8_t> clear_buf(clear_size, 0);
        m_lcd->draw_bitmap(clear_buf.data(), BSP_LCD_H_RES, BSP_LCD_V_RES, 0, 0);
    }

    while (true) {
        EventBits_t event_bits = xEventGroupWaitBits(
            m_event_group, frame_cap::WhoFrameCapNode::NEW_FRAME | TASK_PAUSE | TASK_STOP, pdTRUE, pdFALSE, portMAX_DELAY);
        if (event_bits & TASK_STOP) {
            break;
        } else if (event_bits & TASK_PAUSE) {
            xEventGroupSetBits(m_event_group, TASK_PAUSED);
            EventBits_t pause_event_bits =
                xEventGroupWaitBits(m_event_group, TASK_RESUME | TASK_STOP, pdTRUE, pdFALSE, portMAX_DELAY);
            if (pause_event_bits & TASK_STOP) {
                break;
            } else {
                continue;
            }
        }

        auto fb = m_frame_cap_node->cam_fb_peek(m_peek_index);
        ensure_buffer(fb->width, fb->height);

        if (fb->format == cam::cam_fb_fmt_t::CAM_FB_FMT_RGB565) {
            convert_rgb565_to_gray(reinterpret_cast<const uint8_t *>(fb->buf),
                                   m_gray_buf.data(),
                                   fb->width,
                                   fb->height);
        } else if (fb->format == cam::cam_fb_fmt_t::CAM_FB_FMT_RGB888) {
            convert_rgb888_to_gray(reinterpret_cast<const uint8_t *>(fb->buf),
                                   m_gray_buf.data(),
                                   fb->width,
                                   fb->height);
        } else {
            ESP_LOGW("SimpleGrayLCDDisp", "unsupported framebuffer format");
            continue;
        }

        update_result(fb);
        dl::image::img_t disp_img = {
            .data = m_gray_buf.data(),
            .width = fb->width,
            .height = fb->height,
            .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565,
        };
        detect::draw_detect_results_on_img(disp_img, m_result.det_res, m_palette565);

        m_lcd->draw_bitmap(m_gray_buf.data(), fb->width, fb->height, 0, 0);
    }

    xEventGroupSetBits(m_event_group, TASK_STOPPED);
    vTaskDelete(NULL);
}

} // namespace lcd_disp
} // namespace who

#endif
