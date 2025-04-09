#define _POSIX_C_SOURCE 200809L  // For strdup function declaration

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "network.h"

#define BUFFER_SIZE 2048

// Initialize a socket connection to the server
int init_socket_connection(const char* server_ip, int server_port) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    printf("Initialisation de la connexion à %s:%d\n", server_ip, server_port);
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur lors de la création du socket");
        return -1;
    }
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Adresse invalide ou non supportée");
        close(sockfd);
        return -1;
    }
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Échec de la connexion");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// Close a socket connection
void close_socket_connection(int sockfd) {
    if (sockfd >= 0) {
        close(sockfd);
    } else {
        printf("Tentative de fermeture d'un socket invalide (%d)\n", sockfd);
    }
}

// Send a message to the server and receive a response
char* send_server_message(const char* message, const char* server_ip, int server_port) {
    int sockfd;
    char buffer[BUFFER_SIZE];
    char* response = NULL;
    
    
    // Initialize socket connection
    sockfd = init_socket_connection(server_ip, server_port);
    if (sockfd < 0) {
        printf("Échec de l'initialisation de la connexion\n");
        return NULL;
    }
    
    
    // Create a new buffer with the message plus newline character
    char message_with_newline[BUFFER_SIZE];
    snprintf(message_with_newline, sizeof(message_with_newline), "%s\n", message);
    
    
    // Send the message with newline
    int bytes_sent = send(sockfd, message_with_newline, strlen(message_with_newline), 0);
    if (bytes_sent < 0) {
        perror("Échec de l'envoi");
        close_socket_connection(sockfd);
        return NULL;
    }
    
    
    // Receive the response
    printf("Attente de la réponse du serveur...\n");
    int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        perror("Échec de la réception");
        close_socket_connection(sockfd);
        return NULL;
    }
    
    // Null-terminate the received data
    buffer[bytes_received] = '\0';
    

    // Allocate memory for the response
    response = strdup(buffer);
    if (response == NULL) {
        printf("Erreur d'allocation mémoire pour la réponse\n");
    }
    
    // Close the socket
    close_socket_connection(sockfd);
    
    return response;
}
