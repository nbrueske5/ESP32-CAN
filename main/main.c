#include "rtos.h"
#include "twai.h"
void app_main(void) {
    twai_init_node();
    rtos_init_clock();
    rtos_init_RXPrinting();
    rtos_init_sendMessages(2000);
}