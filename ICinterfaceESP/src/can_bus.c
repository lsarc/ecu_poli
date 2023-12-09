#include "can_bus.h"

void can_bus_init() {
    // Initialize CAN bus interface
}

void can_bus_task(void *pvParameters) {
    while(1) {
        // Send and receive CAN bus messages
        if(can_message_received()) {
            process_can_message();
        }
        // Delay for task loop
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}