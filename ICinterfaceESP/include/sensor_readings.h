#ifndef SENSOR_READINGS_H
#define SENSOR_READINGS_H

#include "freertos/FreeRTOS.h"

// Estrutura para manter a leitura dos sensores
typedef struct {
    float throttle_position; // Posição do acelerador
    float battery_voltage;   // Tensão da bateria
    float battery_current;   // Corrente da bateria
    float motor_speed;       // Velocidade do motor
    float vehicle_speed;     // Velocidade do veículo
    // Outras leituras de sensores conforme necessário
} sensor_values_t;

// Inicializa sensores
void sensors_init(void);

// Tarefa para ler dados dos sensores
void sensor_reading_task(void *pvParameters);

// Função para obter a última leitura de sensores
sensor_values_t get_latest_sensor_values(void);

// Funções específicas para cada sensor, se necessário
float get_throttle_position(void);
float get_battery_status(void);
float get_motor_speed(void);
float get_vehicle_speed(void);

#endif // SENSOR_READINGS_H