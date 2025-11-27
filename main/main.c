#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_adc/adc_continuous.h"

static const char *TAG = "ADC_CONT";

#define ADC_READ_LEN        512   // DMA frame size
#define ADC_BUFFER_SIZE     2048

void app_main(void)
{
    esp_err_t ret;
    adc_continuous_handle_t adc_handle = NULL;

    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = ADC_BUFFER_SIZE,
        .conv_frame_size = ADC_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &adc_handle));

    adc_digi_pattern_config_t pattern = {
        .atten = ADC_ATTEN_DB_12,
        .bit_width = ADC_BITWIDTH_12,
        .channel = ADC_CHANNEL_6,
        .unit = ADC_UNIT_1,
    };

    adc_continuous_config_t cont_cfg = {
        .sample_freq_hz = 20000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
        .pattern_num = 1,
        .adc_pattern = &pattern,
    };
    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &cont_cfg));

    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
    ESP_LOGI(TAG, "ADC continuous started");

    uint8_t rx_buffer[ADC_READ_LEN];
    uint32_t ret_num = 0;


    while (1) {
        ret = adc_continuous_read(adc_handle, rx_buffer, ADC_READ_LEN, &ret_num, 1000);
        
        if (ret == ESP_OK) {
            int sample_count = 0;
            uint32_t sum = 0;
            
            for (int i = 0; i < ret_num; i += sizeof(adc_digi_output_data_t)) {
                adc_digi_output_data_t *p = (adc_digi_output_data_t *)&rx_buffer[i];
                uint8_t ch = p->type2.channel;
                
                if (ch == ADC_CHANNEL_6) {
                    uint16_t raw = p->type2.data;
                    sum += raw;
                    sample_count++;
                }
            }
            
            if (sample_count > 0) {
                ESP_LOGI(TAG, "Average RAW: %lu (%d samples)", sum/sample_count, sample_count);
            }
            
        } else if (ret == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "Timeout");
        } else if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "Buffer overflow!");
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
