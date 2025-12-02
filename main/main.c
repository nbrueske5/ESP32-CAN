#include "rtos.h"
#include "twai.h"
#include "freertos/task.h"

void app_main(void) {
    twai_init_node();
    rtos_init_clock();
    rtos_init_RXPrinting();

    uint8_t msg[] = {1, 0, 4};
    uint8_t msg2[] = {0, 10, 2};
    while (true) {
        twai_transmit(2, msg, sizeof(msg));
        vTaskDelay(pdMS_TO_TICKS(1000));
        twai_transmit(2, msg2, sizeof(msg2));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}