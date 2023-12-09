#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "freertos/FreeRTOS.h"

// Definições de estado do veículo
typedef enum {
    VEHICLE_STATE_INIT,
    VEHICLE_STATE_ELECTRIC_DRIVE,
    VEHICLE_STATE_ICE_GENERATION,
    VEHICLE_STATE_CRUISE,
    VEHICLE_STATE_REGENERATIVE_BRAKING,
    VEHICLE_STATE_MANUAL_OVERRIDE,
    // Outros estados conforme necessário
} vehicle_state_t;

// Funções de controle do veículo
void app_controller_task(void *pvParameters);
void start_ice_generation(void);
void stop_ice_generation(void);
void set_vehicle_to_electric_drive(void);
void apply_regenerative_braking(void);
void manual_override(void); // Em caso de necessidade de controle direto

// Outros protótipos de função conforme necessário para controle de app

#endif // APP_CONTROLLER_H