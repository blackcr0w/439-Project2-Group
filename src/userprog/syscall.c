#include <syscall-nr.h>
#include <stdio.h>
#include <string.h>
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "threads/synch.h"
  
static void syscall_handler (struct intr_frame *);

int exec (const char *cmd_line);
struct semaphore sema_files;  // only allow one file operation at a time

//Spencer driving here
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  sema_init (&sema_files, 1);
}

//Spencer and Jeff driving here
static void
syscall_handler (struct intr_frame *f) 
{
  // only allow one syscall at a time (up at the end)
  struct thread *cur = thread_current ();
  
  // get the system call
  int * esp = f->esp;
  bad_pointer (esp);

  // redirect to the correct function handler
  switch (*esp)
  { 
    case SYS_HALT: // 0
      halt ();
      break;

    case SYS_EXIT: // 1
      bad_pointer (esp+1);
      exit (*(esp+1));  // status
      break;
 
    case SYS_EXEC: // 2  
      bad_pointer (*(esp+1));

      f->eax = exec (*(esp+1));  // cmd_line pointer
      break;
 
    case SYS_WAIT: // 3
      bad_pointer (esp+1);
      f->eax = wait (*(esp+1)); // gets first argument that is pid
      break;  

    case SYS_CREATE:   // 4
      bad_pointer (*(esp+1));
      bad_pointer (esp+2);
      f->eax = create ( *(esp+1), *(esp+2) );  // file char pointer and inital size
      break;

    case SYS_REMOVE:    // 5
       bad_pointer (*(esp+1));
       f->eax = remove (*(esp+1));  // file char pointer
      break;

    case SYS_OPEN:     // 6
      bad_pointer (*(esp+1));   
      f->eax = open (*(esp+1));  // file char pointer
      break;

    case SYS_FILESIZE:  // 7
      bad_pointer (esp+1);
      f->eax = filesize (*(esp+1));  // fd index
      break;

    case SYS_READ:   
      bad_pointer (esp+1);
      bad_pointer (*(esp+2));
      bad_pointer (esp+3);
      f->eax = read ( *(esp+1), *(esp+2), *(esp+3) );  // fd, buffer pointer, size
      break;

    case SYS_WRITE:   
      bad_pointer (esp+1);
      bad_pointer (*(esp+2));
      bad_pointer (esp+3);
      f->eax = write ( *(esp+1), *(esp+2), *(esp+3) ); // fd, buffer pointer, size
      break;
    
    case SYS_SEEK:
      bad_pointer (esp+1);
      bad_pointer (esp+2);
      seek ( *(esp+1), *(esp+2) );
      break;

    case SYS_TELL:
      bad_pointer (esp+1);
      f->eax = tell (*(esp+1)); 
      break;

    case SYS_CLOSE:
      bad_pointer (esp+1);
      close (*(esp+1));
      break;

    case SYS_CHDIR
      
      break;

    case SYS_MKDIR
      break;

    case SYS_READDIR      
      break;

    case SYS_ISDIR            
      break; 

    case SYS_INUMBER   
      break;     

    default: 
      thread_exit ();
      break;
  }   
}

/* Change the current directory. */
bool chdir (const char *dir)
{
  char *path_cpy = path;   // copying path
  char *token, *save_ptr;       // for spliter
  char s[strlen(path_cpy)];

  strlcpy(s, path_cpy, strlen (path_cpy)+1);  //moves path copy into s, add 1 for null

  for (token = strtok_r (s, " ", &save_ptr); token != NULL;
        token = strtok_r (NULL, " ", &save_ptr))
  {    
    // do func with the tokenized thing.

  }
  return false;
}

/* Create a directory. */
bool mkdir (const char *dir)
{

  char *path_cpy = path;   // copying path
  char *token, *save_ptr;       // for spliter
  char s[strlen(path_cpy)];

  strlcpy(s, path_cpy, strlen (path_cpy)+1);  //moves path copy into s, add 1 for null

  for (token = strtok_r (s, " ", &save_ptr); token != NULL;
        token = strtok_r (NULL, " ", &save_ptr))
  {    
    // do func with the tokenized thing.
      
  }

  return true;
}

 /* Reads a directory entry. */
bool readdir (int fd, char *name)
{
  char *path_cpy = path;   // copying path
  char *token, *save_ptr;       // for spliter
  char s[strlen(path_cpy)];

  strlcpy(s, path_cpy, strlen (path_cpy)+1);  //moves path copy into s, add 1 for null

  for (token = strtok_r (s, " ", &save_ptr); token != NULL;
        token = strtok_r (NULL, " ", &save_ptr))
  {    
    // do func with the tokenized thing.
      
  }

  return true;
}

/* Tests if a fd represents a directory. */
bool isdir (int fd)
{
  return false;
}

/* Returns the inode number for a fd. */
int inumber (int fd)
{
  return;
}








/*Closes all the files in a process, called before it exists */
void
close_files()
{
  int counter;
  for(counter = 2; counter < 130; counter++)
    close (counter);   
}        

// helper to check if pointer is mapped, !null and in the user address space
//Cohen and Spencer driving here
void
bad_pointer(int *esp) 
{
  struct thread *cur = thread_current ();
  
  // check if esp is a valid ptr at all
  if(esp == NULL) //need to check for unmapped
  {
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
    thread_exit ();
  }

  // make sure esp is in the user address space
  if(is_kernel_vaddr (esp))
  {
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
    thread_exit ();
  }
    
  // check if esp is unmapped
  if( pagedir_get_page (cur->pagedir, esp) == NULL )
  {
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
    thread_exit ();
  }
}

//Jeff driving here
void 
halt (void)
{
  shutdown_power_off ();
} 
 
// Cohen driving here
void  
exit (int status)
{
  struct thread *cur = thread_current ();

  cur->exit_status = status;

  char *save_ptr; // for spliter  

  printf ("%s: exit(%d)\n", strtok_r(cur->name, " ", &save_ptr), status);

  file_close (cur->save); // closing file so that write can be allowed

  close_files (); // close all open files in a process

  thread_exit (); 
}

// execute the given command line
//Dakota driving here
int 
exec (const char *cmd_line)
{   
  // get string of args and file name, pass into execute
  int tid = process_execute (cmd_line);

  // wait for child to return
  if(tid == -1) // if not loaded correctly
    return -1;
  
  else          // if child loaded correctly
    return tid;      
}

//Spencer driving here
int 
wait (pid_t pid)
{
  return process_wait (pid);  // call process_wait
}

//Dakota driving here
bool 
create (const char *file, unsigned initial_size)
{
  sema_down (&sema_files); // prevent multi-file manipulation
  bool res = filesys_create (file, initial_size);  // save result
  sema_up (&sema_files);   // release file
  return res;
}

//Jeff driving here
bool 
remove (const char *file)
{
  sema_down (&sema_files); // prevent multi-file manipulation
  bool res = filesys_remove (file); // save result
  sema_up (&sema_files); // release file
  return res;
}

//Cohen driving here
int 
open (const char *file)
{
  sema_down (&sema_files); // prevent multi-file manipulation
  
  struct thread *cur = thread_current ();
  cur->file_pointers[cur->fd_index] = filesys_open (file);

  // if file is null -1, else return incremented index
  int res = (cur->file_pointers[cur->fd_index] == NULL) ? -1 : cur->fd_index++;
  
  sema_up (&sema_files); // release file
  return res;
} 

//Spencer driving here
int 
filesize (int fd) 
{
  sema_down (&sema_files); // prevent multi-file manipulation

  // if fd is invalid return 0, otherwise get length of file
  int res = (fd<2 || fd>thread_current ()->fd_index) ? 0 : 
            file_length (thread_current ()->file_pointers[fd]);

  sema_up (&sema_files); // release file
  return res;
}

//Jeff and Spencer driving here
int 
read (int fd, void *buffer, unsigned size)
{  
  sema_down (&sema_files); // prevent multi-file manipulation
  struct thread *cur = thread_current ();
  int res; // save result

  // read from keyboard
  if(fd == 0)
  {
    unsigned i;
    for(i = 0; i < size; i++)
    {
      // cast to char
      char * c_ptr = (char *) buffer;
      *(c_ptr+i) = input_getc ();
    }
    res = size;
  }

  // if fd is valid, read the file  
  else if(fd > 1 && fd <= cur->fd_index && buffer != NULL)
    res = file_read (cur->file_pointers[fd], buffer, size);

  else
    res = -1;

  sema_up (&sema_files); // release file
  return res;
}

//Dakota driving here
int 
write (int fd, const void *buffer, unsigned size)
{
  struct thread * cur = thread_current ();

  if(fd > 0 && fd <= cur->fd_index && buffer != NULL)
  {
    sema_down (&sema_files);

    //write to console
    if(fd==1)
    {
      // if there is nothing to write
      if(size == 0 || buffer == NULL)
      {
        sema_up (&sema_files);
        return 0;
      }    
      putbuf (buffer, size);

      sema_up (&sema_files);
      return size;
    }

    // write to file
    if(fd != 0 && fd != 1)
    {
      sema_up (&sema_files);
      struct thread *cur = thread_current ();

      return file_write (cur->file_pointers[fd], buffer, size);
    }

    // done writing, let other write
    sema_up (&sema_files);
    return size;
  }
  // bad fd
  return 0;
}

//Spencer driving here
void 
seek (int fd , unsigned position)
{
  sema_down (&sema_files); // prevent multi-file manipulation
  if((fd>=2 && fd<=thread_current ()->fd_index))
  {
    file_seek (thread_current ()->file_pointers[fd], position);
    sema_up (&sema_files); // release file
  }
  else
  {
    sema_up (&sema_files); // release file
    thread_exit ();
  }
}

//Cohen driving here
unsigned  
tell (int fd)
{
  sema_down (&sema_files); // prevent multi-file manipulation

  unsigned res;

  if(fd<2 || fd>thread_current ()->fd_index)
  {
    sema_up (&sema_files); // release file
    thread_exit ();
  }

  res = file_tell (thread_current ()->file_pointers[fd]);
  sema_up (&sema_files); // release file
  return res;
}
 
//Dakota driving here
void 
close (int fd)
{
  sema_down (&sema_files); // prevent multi-file manipulation
  
  struct thread * t = thread_current ();  // get the current thread
  
  if(fd>=2 && fd<=t->fd_index)           // check fd is valid
    if(t->file_pointers[fd] != NULL)     // check that it is not closed
    { 
      file_close (t->file_pointers[fd]); // close the file
     
      // keep closed file from closing
      t->file_pointers[fd] = NULL;      
    }
  // else fail silently
  sema_up (&sema_files); // release file
}

void dir_pieces (char *path, function *func)
{
  char *path_cpy = path;   // copying path
  char *token, *save_ptr;       // for spliter
  char s[strlen(path_cpy)];

  strlcpy(s, path_cpy, strlen (path_cpy)+1);  //moves path copy into s, add 1 for null

  for (token = strtok_r (s, " ", &save_ptr); token != NULL;
        token = strtok_r (NULL, " ", &save_ptr))
  {    
    // do func with the tokenized thing.
    func (token);
  }
}
