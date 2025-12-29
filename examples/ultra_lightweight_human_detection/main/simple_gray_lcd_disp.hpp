#pragma once

#include "who_detect.hpp"
#include "who_frame_cap_node.hpp"
#include "who_task.hpp"
#include "who_lcd.hpp"

#include <cstdint>
#include <queue>
#include <vector>

#if BSP_CONFIG_NO_GRAPHIC_LIB
namespace who {
namespace lcd_disp {

class SimpleGrayLCDDisp : public task::WhoTask {
public:
    SimpleGrayLCDDisp(const std::string &name,
                      frame_cap::WhoFrameCapNode *frame_cap_node,
                      const std::vector<std::vector<uint8_t>> &palette,
                      int peek_index = 0);
    ~SimpleGrayLCDDisp() override;

    void save_detect_result(const detect::WhoDetect::result_t &result);
    void cleanup_results();

private:
    void task() override;
    void update_result(const cam::cam_fb_t *fb);
    void ensure_buffer(uint16_t width, uint16_t height);
    void convert_rgb565_to_gray(const uint8_t *src, uint8_t *dst, int width, int height);
    void convert_rgb888_to_gray(const uint8_t *src, uint8_t *dst, int width, int height);

    lcd::WhoLCD *m_lcd;
    frame_cap::WhoFrameCapNode *m_frame_cap_node;
    int m_peek_index;
    std::vector<uint8_t> m_gray_buf;
    std::vector<std::vector<uint8_t>> m_palette565;
    SemaphoreHandle_t m_res_mutex;
    std::queue<detect::WhoDetect::result_t> m_results;
    detect::WhoDetect::result_t m_result;
};

} // namespace lcd_disp
} // namespace who
#endif
