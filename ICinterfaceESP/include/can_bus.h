#ifndef CAN_BUS_H
#define CAN_BUS_H

#include "freertos/FreeRTOS.h"

// Inicializa a interface CAN
void can_bus_init(void);

// Tarefa que gerencia a comunicação CAN
void can_bus_task(void *pvParameters);

// Envia uma mensagem CAN
void can_send_message(const uint32_t id, const char *data, const uint8_t length);

// Processa mensagens CAN recebidas
void process_can_message(void);

// Verifica se uma mensagem CAN foi recebida
bool can_message_received(void);

#endif // CAN_BUS_H