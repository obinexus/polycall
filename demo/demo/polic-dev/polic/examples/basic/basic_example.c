/**
 * @file basic_example.c
 * @brief Basic example of using PoliC
 */

#include <polic/polic.h>
#include <stdio.h>

void send_net_data() {
    printf("Sending data over the network...\n");
}

void secured_send_net_data() {
    /* This would be implemented using the PoliC API */
    printf("Secured function called\n");
}

int main() {
    /* Initialize PoliC with sandbox mode enabled and blocking policy */
    polic_init(true, POLIC_BLOCK);
    
    /* In a complete implementation, this would use the POLIC_DECORATOR macro */
    secured_send_net_data();
    
    return 0;
}
