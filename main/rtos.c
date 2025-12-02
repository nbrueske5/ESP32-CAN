#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "esp_log.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "twai.h"

const char *TAG = "RTOS";

TaskHandle_t printRxData_hdl = NULL;
TaskHandle_t sendMessages_hdl = NULL;
TaskHandle_t clock_hdl = NULL;

/**
 * FreeRTOS task. Prints and eats any message added to RXQueue when added by the Twai RX Callback function (static bool twai_rx_cb() in twai.c)
 * useful for testing!
 */
static void printRXData(void *parameters) {
    twai_message_t message_frame;
    uint8_t message[8] = {0};
    for(;;) {
        if (xQueueReceive(rxQueue_hdl, &message_frame, pdMS_TO_TICKS(100)) == pdTRUE) {
            ESP_LOGI(TAG, "ID: %d, DLC: %d, message: ", message_frame.identifier, message_frame.data_length_code);
            // store message in
            if (message_frame.data_length_code > 0) {
                for (int i = 0; i < message_frame.data_length_code; i++) {
                    message[i] = message_frame.data[i];
                    printf("%x", message[i]);
                }
                printf("\r\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // (experimental value) let the CPU breath :D
    }
}

/**
 * Task that sends a message with increasing ID ever 100 milliseconds - useful for debugging and testing filters
 * @param *paramters - and integer value to determine how many messages to send
 */
static void send_messages(void *parameters) {
    for (int i = 0; i < (int)parameters; i++) {
        twai_transmit((uint32_t)i, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    };
    vTaskDelete(NULL); //delete this task when done
} 

/**
 * Task that counts upwards every second
 */
static void start_clock(void *parameters) {
    int sec = 0;
    for (;;) {
        ESP_LOGI(TAG, "------------------------ %ds ------------------------", sec);
        sec++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * Creates a task to continuosly check the twai RX Queue and print any message found there
 */
void rtos_init_RXPrinting(void) {
    
    xTaskCreate(
        printRXData,    //function name
        "print RX data",    //task name
        5000,   //task stack size
        NULL,   //task parameters
        10,  //task priority
        &printRxData_hdl
    );
    ESP_LOGI(TAG, "created RX Printing task");
}

/**
 * Creates a task to continuosly send twai messages with increasing ID's (good for checking filters and such!)
 */
void rtos_init_sendMessages(int numMessages) {
    
    xTaskCreate(
        send_messages,    //function name
        "send messages",    //task name
        5000,   //task stack size
        (void*)numMessages,   //task parameters
        5,  //task priority
        &sendMessages_hdl
    );
    ESP_LOGI(TAG, "created sending messages task");
}

/**
 * Creates a task which adds as a simple clock. Prints seconds past since creation
 */
void rtos_init_clock(void) {
    xTaskCreate(
        start_clock,    //function name
        "clock",    //task name
        5000,   //task stack size
        NULL,   //task parameters
        1,  //task priority
        &clock_hdl
    );
    ESP_LOGI(TAG, "started the clock");
}