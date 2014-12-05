#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>

char dir_path [500]; // limit size of directory path to 500 characters
char *stop_last;

// define pid_t
typedef int pid_t;
 
void close_files(void);
void bad_pointer(int* esp);
void syscall_init (void);
void halt (void);
void exit (int status);
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


#endif /* userprog/syscall.h */
