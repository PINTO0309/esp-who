#include "frame_cap_pipeline.hpp"
#include "uhd_detect.hpp"
#include "who_detect_app_lcd.hpp"
#include "bsp/esp-bsp.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdio>
#include <cctype>
#include <cstring>

using namespace who::frame_cap;
using namespace who::app;

namespace {
static const char *TAG = "UHD_MAIN";
constexpr int kCliTimeoutMs = 2000;

who::uhd::InputMode parse_input_mode(const char *value)
{
    if (!value) {
        return who::uhd::InputMode::RGB888;
    }
    char buf[32] = {0};
    size_t len = 0;
    for (size_t i = 0; value[i] != '\0' && len < sizeof(buf) - 1; i++) {
        if (std::isalnum(static_cast<unsigned char>(value[i]))) {
            buf[len++] = static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
        } else if (value[i] == '_' || value[i] == '-') {
            buf[len++] = '_';
        }
    }
    buf[len] = '\0';

    if (strcmp(buf, "rgb") == 0 || strcmp(buf, "rgb888") == 0) {
        return who::uhd::InputMode::RGB888;
    }
    if (strcmp(buf, "yuv") == 0 || strcmp(buf, "yuv422") == 0) {
        return who::uhd::InputMode::YUV422;
    }
    if (strcmp(buf, "y") == 0 || strcmp(buf, "yonly") == 0 || strcmp(buf, "y_only") == 0) {
        return who::uhd::InputMode::Y_ONLY;
    }
    if (strcmp(buf, "yternary") == 0 || strcmp(buf, "y_ternary") == 0) {
        return who::uhd::InputMode::Y_TERNARY;
    }
    if (strcmp(buf, "ybinary") == 0 || strcmp(buf, "y_binary") == 0) {
        return who::uhd::InputMode::Y_BINARY;
    }
    return who::uhd::InputMode::RGB888;
}

const char *input_mode_to_string(who::uhd::InputMode mode)
{
    switch (mode) {
    case who::uhd::InputMode::RGB888:
        return "rgb888";
    case who::uhd::InputMode::YUV422:
        return "yuv422";
    case who::uhd::InputMode::Y_ONLY:
        return "y_only";
    case who::uhd::InputMode::Y_TERNARY:
        return "y_ternary";
    case who::uhd::InputMode::Y_BINARY:
        return "y_binary";
    default:
        return "rgb888";
    }
}

who::uhd::InputMode read_input_mode_from_uart()
{
    esp_err_t err = uart_driver_install(UART_NUM_0, 256, 0, 0, nullptr, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "uart_driver_install failed: %d", err);
    }

    printf("input-mode? (rgb888|yuv422|y_only|y_ternary|y_binary) [rgb888]: ");
    fflush(stdout);

    char buf[64] = {0};
    size_t len = 0;
    int64_t start = esp_timer_get_time();

    while (((esp_timer_get_time() - start) / 1000) < kCliTimeoutMs) {
        uint8_t ch = 0;
        int ret = uart_read_bytes(UART_NUM_0, &ch, 1, pdMS_TO_TICKS(10));
        if (ret <= 0) {
            continue;
        }
        if (ch == '\n' || ch == '\r') {
            break;
        }
        if (len < sizeof(buf) - 1) {
            buf[len++] = static_cast<char>(ch);
        }
    }

    buf[len] = '\0';
    return parse_input_mode(buf);
}
} // namespace

extern "C" void app_main(void)
{
    vTaskPrioritySet(xTaskGetCurrentTaskHandle(), 5);

// close led
#ifdef BSP_BOARD_ESP32_S3_EYE
    ESP_ERROR_CHECK(bsp_leds_init());
    ESP_ERROR_CHECK(bsp_led_set(BSP_LED_GREEN, false));
#endif

    auto input_mode = read_input_mode_from_uart();
    ESP_LOGI(TAG, "input mode: %s", input_mode_to_string(input_mode));

    auto frame_cap = get_frame_cap_pipeline();
    auto detect_app = new WhoDetectAppLCD({{255, 0, 0}}, frame_cap);
    auto detect_model = new who::uhd::UHDDetect(input_mode);
    detect_app->set_model(detect_model);
    detect_app->run();
}
