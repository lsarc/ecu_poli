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
#include "estimador.h"


#define SYNC_TASK_PRIO 1
#define SAMPLE_TASK_PRIO 3
#define CONTROL_TASK_PRIO 2

const static char *TAG = "ecu_main";

#define CONT_HIGH_LIMIT 5000
#define CONT_LOW_LIMIT  -5000

#define CONT_GPIO_EDGE 25
#define CONT_GPIO_LEVEL 34

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

#define ADC1_CHAN0          ADC_CHANNEL_4

#define ADC_ATTEN           ADC_ATTEN_DB_11

#define TIMER_RES 10000
#define ALARM_COUNT 2000
const int MEAS_FREQ = TIMER_RES/ALARM_COUNT;

static float estimatorArray[SAMPLING_SIZE];
static float plantArray[SAMPLING_SIZE];

static ring_buffer plantBuffer;
static ring_buffer estimatorBuffer;

// SAÍDA DO PWM



// TASK DE MEDIÇÃO DE ROTAÇÃO

static void sample_task(void *args[])
{
    pcnt_unit_handle_t pcnt_unit = args[1];
    QueueHandle_t queue = args[2];
    int event_count, pulse_count;
    float vel;    
    while (1) 
    {
        if (xQueueReceive(queue, &event_count, portTICK_PERIOD_MS)) {
            ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
            pcnt_unit_clear_count(pcnt_unit);
            vel = pulse_count*MEAS_FREQ/4;
            updateSamples(&plantBuffer, &estimatorBuffer, vel, estimator(&plantBuffer, &estimatorBuffer, &plantArray, &estimatorArray, 1.0));
            ESP_LOGI(TAG, "%f Hz", vel); 
            vTaskDelay(portTICK_PERIOD_MS);
        } 
    }
}

// TASK DE CONTROLE

static void control_task(void *args[])
{
    int pwm = 4095;
    while(1)
    {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, pwm));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        vTaskDelay(portTICK_PERIOD_MS);
    }
}

// TASK DE SINCRONIZAÇÃO

static void sync_task(void *args[])
{
    void *adc1_handler = args[0];
    xTaskCreatePinnedToCore(sample_task, "sample", 4096, args, SAMPLE_TASK_PRIO, NULL, 1);
    xTaskCreatePinnedToCore(control_task, "control", 4096, args, CONTROL_TASK_PRIO, NULL, 0);
    ESP_LOGI(TAG, "ALL TASKS STARTED");
    while(1)
    {
        vTaskDelay(portTICK_PERIOD_MS);
    }
}

// INTERRUPÇÃO DO ALARME DO TIMER

static bool alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    xQueueSendFromISR(queue, &(edata->count_value), &high_task_awoken);
    return high_task_awoken == pdTRUE;
}

// PROGRAMA INICIA AQUI

void app_main(void)
{
    // CONTADOR PULSOS

    ESP_LOGI(TAG, "install pcnt unit");
    pcnt_unit_config_t unit_config = {
        .high_limit = CONT_HIGH_LIMIT,
        .low_limit = CONT_LOW_LIMIT,
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
        .edge_gpio_num = CONT_GPIO_EDGE,
        .level_gpio_num = CONT_GPIO_LEVEL,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = CONT_GPIO_LEVEL,
        .level_gpio_num = CONT_GPIO_EDGE,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    // SETUP DO ROTARY ENCODER PARA DETECÇÃO DE SENTIDO

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

    // ALARME PARA INTERRUPÇÃO DA MEDIÇÃO DE ROTAÇÃO

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

    // LEDC / USADO PARA O PWM

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


    // LEITURA ADC

    // ADC1 Init
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // ADC1 Config 
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHAN0, &config));

    // INICIALIZA VETORES E BUFFERS

    for(int i = 0; i < SAMPLING_SIZE; i++)
    {
        write(0.0, &estimatorBuffer);
        write(0.0, &plantBuffer);
    }
    fillAbsoluteVectors(&plantArray, &estimatorArray);

    // VETOR DE ARGUMENTOS

    void* args[3];
    args[0] = adc1_handle;
    args[1] = pcnt_unit;
    args[2] = queue;

    // INICIA TASK DE SINCRONIZAÇÃO

    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "STARTING TASKS");
    xTaskCreatePinnedToCore(sync_task, "sync", 4096, args, SYNC_TASK_PRIO, NULL, 0);

}

