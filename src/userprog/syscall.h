#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include <syscall-nr.h>
#include <stdio.h>
#include <string.h>
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.c"
#include "filesys/directory.c"
#include "filesys/file.c"


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

char * get_cmd_line(char * cmd_line);
bool chdir (const char *dir);
bool mkdir (const char *dir);
bool readdir (int fd, char *name);
bool isdir (int fd);
int inumber (int fd);
char *get_last (char * path);
bool valid_mkdir (char *dir, struct dir *dir_to_add);

#endif /* userprog/syscall.h */
