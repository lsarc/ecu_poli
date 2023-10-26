/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "driver/gptimer.h"

#define SYNC_TASK_PRIO 1
#define LED_TASK_PRIO 2
#define COUNT_TASK_PRIO 3

const static char *TAG = "ecu_main";

#define EXAMPLE_PCNT_HIGH_LIMIT 5000
#define EXAMPLE_PCNT_LOW_LIMIT  -5000

#define EXAMPLE_CONT_GPIO_EDGE 25
#define EXAMPLE_CONT_GPIO_LEVEL 34

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (2) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (1000) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz
/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
//ADC1 Channels

#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_4

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_11

static int adc_raw;

#define TIMER_RES 10000
#define ALARM_COUNT 2000
const int MEAS_FREQ = TIMER_RES/ALARM_COUNT;


static void led_dim_task(void *arg)
{
    adc_oneshot_unit_handle_t adc1_handle = arg;
    while(1)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw));
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, adc_raw);
        int dim = (adc_raw-700)>>4;
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, dim*38));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        vTaskDelay(portTICK_PERIOD_MS);
    }
    
}

static void speed_meas_task(void *arg[])
{
    pcnt_unit_handle_t pcnt_unit = arg[1];
    QueueHandle_t queue = arg[2];
    int event_count, pulse_count;    
    while (1) 
    {
        if (xQueueReceive(queue, &event_count, portTICK_PERIOD_MS)) {
            ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
            pcnt_unit_clear_count(pcnt_unit);
            ESP_LOGI(TAG, "%d Hz", pulse_count*MEAS_FREQ/4); 
            vTaskDelay(portTICK_PERIOD_MS);
        } 
    }
}

static void sync_task(void *args[])
{
    void* adc1_handler = args[0];
    xTaskCreatePinnedToCore(led_dim_task, "led_dim", 4096, adc1_handler, LED_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(speed_meas_task, "speed", 4096, args, COUNT_TASK_PRIO, NULL, tskNO_AFFINITY);
    
    while(1)
    {
        vTaskDelay(portTICK_PERIOD_MS);
    }
}

static bool alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    xQueueSendFromISR(queue, &(edata->count_value), &high_task_awoken);
    return high_task_awoken == pdTRUE;
}

void app_main(void)
{
    // CONTADOR PULSOS

    ESP_LOGI(TAG, "install pcnt unit");
    pcnt_unit_config_t unit_config = {
        .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
        .low_limit = EXAMPLE_PCNT_LOW_LIMIT,
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    ESP_LOGI(TAG, "set glitch filter");
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    ESP_LOGI(TAG, "install pcnt channels");
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = EXAMPLE_CONT_GPIO_EDGE,
        .level_gpio_num = EXAMPLE_CONT_GPIO_LEVEL,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = EXAMPLE_CONT_GPIO_LEVEL,
        .level_gpio_num = EXAMPLE_CONT_GPIO_EDGE,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    ESP_LOGI(TAG, "set edge and level actions for pcnt channels");
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_LOGI(TAG, "enable pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_LOGI(TAG, "clear pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_LOGI(TAG, "start pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    // TIMER

    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RES, // 10KHz, 1 tick = 0.1ms
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0, // counter will reload with 0 on alarm event
        .alarm_count = ALARM_COUNT, // period = 0.2s @resolution 10KHz
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = alarm_cb, // register user callback
    };
    QueueHandle_t queue = xQueueCreate(10, sizeof(int));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, queue));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    //LED

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = LEDC_DUTY, // Set duty to 50%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));


    //LEITURA ADC

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = EXAMPLE_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));

    void* args[3] ;
    args[0] = adc1_handle;
    args[1] = pcnt_unit;
    args[2] = queue;

    vTaskDelay(pdMS_TO_TICKS(100));
    xTaskCreatePinnedToCore(sync_task, "sync", 4096, args, LED_TASK_PRIO, NULL, tskNO_AFFINITY);
    
    //LOOP PRINCIPAL
    
    
    //xTaskCreatePinnedToCore(speed, "speed", 4096, NULL, STATS_TASK_PRIO, NULL, tskNO_AFFINITY);
    
    /* while (1) {

        if (xQueueReceive(queue, &dim, pdMS_TO_TICKS(tempo_espera_ms))) {
            ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
            pcnt_unit_clear_count(pcnt_unit);
            ESP_LOGI(TAG, "%d Hz", pulse_count*5/4); // pulsos no tempo/tempo de amostragem * 60 / 4 
            
        } 

        vTaskDelay(pdMS_TO_TICKS(tempo_espera_ms));
    } */
    
    
}

