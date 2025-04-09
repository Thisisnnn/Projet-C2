#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

char * get_username() {
    // This function returns the username of the current user
    char *username = malloc(256);
    if (getlogin_r(username, 256) == -1) {
        perror("username");
        free(username);
        return NULL;
    }
    return username;
}

char * get_hostname() {
    // This function returns the hostname of the machine
    char *hostname = malloc(256);
    if (gethostname(hostname, 256) == -1) {
        perror("hostname");
        free(hostname);
        return NULL;
    }
    return hostname;
}


char * get_os() {
    // This function returns the OS name
    char *os = malloc(256);
    snprintf(os, 256, "Linux"); // Placeholder for actual OS detection
    return os;
}

int main() {
    // Send a message to the server
    const char *message = "Hello, server!";

    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    };

    // Set up the server address structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);


    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Send the message
    if (send(sock, message, strlen(message), 0) == -1) {
        perror("send");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Message sent: %s\n", message);

    //Close the socket
    close(sock);
    return EXIT_SUCCESS;
}