#include "dl_model_base.hpp"
#include "dl_tensor_base.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <algorithm>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {
constexpr char kTag[] = "BENCH";

#ifndef MODEL_INPUT_W
#define MODEL_INPUT_W 64
#endif
#ifndef MODEL_INPUT_H
#define MODEL_INPUT_H 64
#endif
#ifndef MODEL_INPUT_C
#define MODEL_INPUT_C 3
#endif
#ifndef MODEL_INPUT_EXP
#define MODEL_INPUT_EXP -7
#endif

#ifndef BENCH_WARMUP
#define BENCH_WARMUP 1
#endif
#ifndef BENCH_ITERS
#define BENCH_ITERS 10
#endif

#ifndef MODEL_SYMBOL
#define MODEL_SYMBOL fastestdetnext_x3_00_x1_00_64x64_opencv_inter_nearest_cls09_espdl
#endif

#define SYMBOL_JOIN(a, b) a##b
#define SYMBOL_MAKE(a, b) SYMBOL_JOIN(a, b)
#define MODEL_START SYMBOL_MAKE(SYMBOL_MAKE(_binary_, MODEL_SYMBOL), _start)
#define MODEL_END SYMBOL_MAKE(SYMBOL_MAKE(_binary_, MODEL_SYMBOL), _end)

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

extern "C" {
extern const uint8_t MODEL_START[];
extern const uint8_t MODEL_END[];
}

struct ModelBlob {
    const uint8_t *data = nullptr;
    size_t size = 0;
    bool owned = false;
};

ModelBlob load_model_blob()
{
    ModelBlob blob;
    const uint8_t *start = MODEL_START;
    const uint8_t *end = MODEL_END;
    if (start == nullptr || end == nullptr || end <= start) {
        ESP_LOGE(kTag, "model symbol is invalid");
        return blob;
    }

    blob.size = static_cast<size_t>(end - start);
    blob.data = start;

    uintptr_t addr = reinterpret_cast<uintptr_t>(start);
    if ((addr & 0x0F) != 0) {
        uint8_t *aligned = static_cast<uint8_t *>(
            heap_caps_aligned_alloc(16, blob.size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!aligned) {
            aligned = static_cast<uint8_t *>(heap_caps_aligned_alloc(16, blob.size, MALLOC_CAP_8BIT));
        }
        if (!aligned) {
            ESP_LOGE(kTag, "failed to allocate aligned model buffer (%zu bytes)", blob.size);
            blob.data = nullptr;
            blob.size = 0;
            return blob;
        }
        std::memcpy(aligned, start, blob.size);
        blob.data = aligned;
        blob.owned = true;
        ESP_LOGW(kTag, "model data copied to aligned buffer");
    }

    return blob;
}

void free_model_blob(ModelBlob &blob)
{
    if (blob.owned && blob.data) {
        heap_caps_free(const_cast<uint8_t *>(blob.data));
    }
    blob.data = nullptr;
    blob.size = 0;
    blob.owned = false;
}

struct Stats {
    double avg_us = 0.0;
    int64_t min_us = 0;
    int64_t max_us = 0;
    int64_t p50_us = 0;
    int64_t p90_us = 0;
};

Stats compute_stats(const std::vector<int64_t> &samples)
{
    Stats stats;
    if (samples.empty()) {
        return stats;
    }

    int64_t sum = 0;
    stats.min_us = samples[0];
    stats.max_us = samples[0];
    for (int64_t value : samples) {
        sum += value;
        if (value < stats.min_us) {
            stats.min_us = value;
        }
        if (value > stats.max_us) {
            stats.max_us = value;
        }
    }
    stats.avg_us = static_cast<double>(sum) / static_cast<double>(samples.size());

    std::vector<int64_t> sorted(samples);
    std::sort(sorted.begin(), sorted.end());
    const size_t last = sorted.size() - 1;
    const size_t idx50 = static_cast<size_t>(0.50 * static_cast<double>(last));
    const size_t idx90 = static_cast<size_t>(0.90 * static_cast<double>(last));
    stats.p50_us = sorted[idx50];
    stats.p90_us = sorted[idx90];

    return stats;
}

} // namespace

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "model=%s", STRINGIFY(MODEL_SYMBOL));

    ModelBlob blob = load_model_blob();
    if (!blob.data || blob.size == 0) {
        ESP_LOGE(kTag, "model blob is empty");
        return;
    }

    dl::Model *model = new dl::Model(reinterpret_cast<const char *>(blob.data),
                                     fbs::MODEL_LOCATION_IN_FLASH_RODATA,
                                     0,
                                     dl::MEMORY_MANAGER_GREEDY,
                                     nullptr,
                                     blob.owned);
    if (!model) {
        ESP_LOGE(kTag, "model allocation failed");
        free_model_blob(blob);
        return;
    }

    dl::TensorBase *model_input = model->get_input();
    if (!model_input) {
        ESP_LOGE(kTag, "model input missing");
        delete model;
        free_model_blob(blob);
        return;
    }

    std::vector<int> shape = model_input->shape;
    dl::dtype_t dtype = model_input->dtype;
    int exponent = model_input->exponent;

    if (shape.empty()) {
        shape = {1, MODEL_INPUT_H, MODEL_INPUT_W, MODEL_INPUT_C};
    }

    ESP_LOGI(kTag,
             "input shape=%s dtype=%s exp=%d",
             dl::vector_to_string(shape).c_str(),
             dl::dtype_to_string(dtype),
             exponent);

    size_t element_count = 1;
    for (int dim : shape) {
        if (dim <= 0) {
            ESP_LOGE(kTag, "invalid input shape dimension: %d", dim);
            delete model;
            free_model_blob(blob);
            return;
        }
        element_count *= static_cast<size_t>(dim);
    }

    void *input_ptr = nullptr;
    std::vector<int8_t> input_i8;
    std::vector<int16_t> input_i16;
    std::vector<float> input_f32;

    switch (dtype) {
    case dl::DATA_TYPE_INT8:
    case dl::DATA_TYPE_UINT8:
        input_i8.assign(element_count, 0);
        input_ptr = input_i8.data();
        break;
    case dl::DATA_TYPE_INT16:
    case dl::DATA_TYPE_UINT16:
        input_i16.assign(element_count, 0);
        input_ptr = input_i16.data();
        break;
    case dl::DATA_TYPE_FLOAT:
        input_f32.assign(element_count, 0.0f);
        input_ptr = input_f32.data();
        break;
    default:
        ESP_LOGE(kTag, "unsupported input dtype: %s", dl::dtype_to_string(dtype));
        delete model;
        free_model_blob(blob);
        return;
    }

    dl::TensorBase input_tensor(shape, input_ptr, exponent, dtype, false);

    ESP_LOGI(kTag, "warmup start (iters=%d)", BENCH_WARMUP);
    for (int i = 0; i < BENCH_WARMUP; ++i) {
        ESP_LOGI(kTag, "warmup %d/%d", i + 1, BENCH_WARMUP);
        model->run(&input_tensor);
    }
    ESP_LOGI(kTag, "warmup done");

    int64_t single_t0 = esp_timer_get_time();
    model->run(&input_tensor);
    int64_t single_t1 = esp_timer_get_time();
    ESP_LOGI(kTag, "single infer: %.2fms", (single_t1 - single_t0) / 1000.0);

    std::vector<int64_t> samples;
    samples.reserve(BENCH_ITERS);
    for (int i = 0; i < BENCH_ITERS; ++i) {
        int64_t t0 = esp_timer_get_time();
        model->run(&input_tensor);
        int64_t t1 = esp_timer_get_time();
        samples.push_back(t1 - t0);
    }

    Stats stats = compute_stats(samples);
    ESP_LOGI(kTag,
             "infer: iters=%d warmup=%d avg=%.2fms min=%.2fms max=%.2fms p50=%.2fms p90=%.2fms",
             BENCH_ITERS,
             BENCH_WARMUP,
             stats.avg_us / 1000.0,
             stats.min_us / 1000.0,
             stats.max_us / 1000.0,
             stats.p50_us / 1000.0,
             stats.p90_us / 1000.0);

    delete model;
    free_model_blob(blob);
}
