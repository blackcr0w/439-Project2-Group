#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

// get bool stuff
#include <stdbool.h>

// define pid_t
typedef int pid_t;

void syscall_init (void);
void halt (void);
void exit (int status);
//int exec_s (const char *cmd_line, uint32_t *eax);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

char * get_cmd_line(char * cmd_line);

#endif /* userprog/syscall.h */
