#ifndef CLIENT_H
#define CLIENT_H

// Global variables declarations
extern char uid[64];
extern double sleep_time;
extern double jitter;

// Function declarations
char* get_username();
char* get_hostname();
char* get_os();
void get_uid();
void check_commands();
void calculate_random_sleep_time();
void execute_server_command(const char *command, char *result, size_t result_size);

#endif // CLIENT_H