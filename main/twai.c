#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "esp_log.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

twai_node_handle_t node_hdl = NULL;
QueueHandle_t rxQueue_hdl = NULL;

static const char *TAG = "TWAI";

/**
 * Twai RX callbakc function. Takes the twai frame, stores it into a twai message and adds it to the rxQueue
 * use a freeRTOS task to process the data (ex: printRXData() in rtos.c)
 */
static bool twai_rx_cb(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx)
{
    uint8_t recv_buff[8];
    twai_frame_t rx_frame = {
        .buffer = recv_buff,
        .buffer_len = sizeof(recv_buff),
    };

    BaseType_t xHigherPriorityTaskWoken = pdFALSE; // context switch is determined by xQueueSendFromISR(). Not 100% sure how this works yet :(
    if (ESP_OK == twai_node_receive_from_isr(handle, &rx_frame)) {
        // create the message to add to queue
        twai_message_t msg = {
            .identifier = rx_frame.header.id,
            .data_length_code = rx_frame.header.dlc,
        };
        memcpy(msg.data, recv_buff, rx_frame.header.dlc);
        xQueueSendFromISR(rxQueue_hdl, &msg, &xHigherPriorityTaskWoken);
    }
    return xHigherPriorityTaskWoken;
}

/**
 * Initialize the node and it's associated RX message queue
 */
void twai_init_node(void) {
    // initialize twai node
    twai_onchip_node_config_t node_config = {
        .io_cfg.tx = 4,             // TWAI TX GPIO pin
        .io_cfg.rx = 5,             // TWAI RX GPIO pin
        .bit_timing.bitrate = 500000,  // 500 kbps bitrate - BMS
        .tx_queue_depth = 5,        // Transmit queue depth set to 5
        .flags = {
            //to enable self testing: 
            .enable_loopback = 1,
            .enable_self_test = 1,
        }
    };

    // Create a new TWAI controller driver instance
    ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &node_hdl));

    // configure receive filter
    // current setup filters out even IDs
    twai_mask_filter_config_t mfilter_cfg = {
    .id = 0,    /**< Single base ID for filtering */
    .mask = 0,      /**< Mask to determine the matching bits (1 = match bit, 0 = any bit) */
    .is_ext = true,    // Accept standard and extended IDs
    };
    ESP_ERROR_CHECK(twai_node_config_mask_filter(node_hdl, 0, &mfilter_cfg));   // Configure on filter 0

    //register receive event callback
    twai_event_callbacks_t user_cbs = {
        .on_rx_done = twai_rx_cb,
    };
    ESP_ERROR_CHECK(twai_node_register_event_callbacks(node_hdl, &user_cbs, NULL));
    
    rxQueue_hdl = xQueueCreate(20, sizeof(twai_message_t));  //create the queue for rx

    // Start the TWAI controller
    ESP_ERROR_CHECK(twai_node_enable(node_hdl)); 
    ESP_LOGI(TAG, "created and started the TWAI controller");
}

/**
 * transmits a (max 8 byte) message through twai (can) communication
 * @param: id - the identifier of the message, tx_buff - a pointer to a buffer with the message, tx_buff_size - size of the message (bytes)
 * @return: esp_err_t if invalid message or node fails to transmit
 */
esp_err_t twai_transmit(uint32_t id, uint8_t* tx_buff, uint8_t tx_buff_size) {
    // error checks
    if (tx_buff_size > 8 || id > 0x1FFFFFFF) {
        return ESP_ERR_INVALID_SIZE;
    };
    if (tx_buff_size > 0 && tx_buff == NULL) {
        return ESP_ERR_INVALID_ARG;
    };

    // build and transmit the message
    twai_frame_t tx_msg = {
        .header.id = id, //message id
        .header.ide = (id > 0x7FF), //use extended id if id > 11 bits
        .header.dlc = tx_buff_size, //message data length code (ESP32 does not support FD format)
        .buffer = tx_buff,
    };

    return twai_node_transmit(node_hdl, &tx_msg, 0);  // Timeout = 0: returns immediately if queue is full
}
