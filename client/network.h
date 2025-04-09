#ifndef NETWORK_H
#define NETWORK_H

// Function declarations for socket-based communication
char* send_server_message(const char* message, const char* server_ip, int server_port);
int init_socket_connection(const char* server_ip, int server_port);
void close_socket_connection(int sockfd);

#endif // NETWORK_H
