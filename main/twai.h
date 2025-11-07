#ifndef TWAI_H
#define TWAI_H

#include <esp_err.h>
#include <esp_twai_types.h>
#include "freertos/FreeRTOS.h"
#include <freertos/queue.h>

extern twai_node_handle_t node_hdl;
extern QueueHandle_t rxQueue_hdl;

void twai_init_node(void);
esp_err_t twai_transmit(uint32_t id, uint8_t* tx_buff, uint8_t tx_buff_size);

#endif // TWAI_WRAP_H