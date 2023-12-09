#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_controller.h"
#include "can_bus.h"
#include "sensor_readings.h"

void app_main() {
    // Initialize CAN Bus
    can_bus_init();

    // Start tasks for sensor reading, actuator control, and CAN bus management
    xTaskCreatePinnedToCore(sensor_reading_task, "sensor_task", 2048, NULL, 10, NULL, 1);
    xTaskCreatePinnedToCore(can_bus_task, "can_bus_task", 2048, NULL, 10, NULL, 1);
    xTaskCreatePinnedToCore(app_controller_task, "app_ctrl_task", 2048, NULL, 10, NULL, 1);

    // ... Other initializations if needed
}