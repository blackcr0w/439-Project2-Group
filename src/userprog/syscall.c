#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "threads/synch.h"
#include "process.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

int exec (const char *cmd_line);
struct semaphore sema_exec;  // binary semaphore to control access to exec function
//struct semaphore sema_pwait;
struct semaphore sema_write;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  sema_init(&sema_exec, 1);

  sema_init(&sema_write, 1);
}

static void
syscall_handler (struct intr_frame *f) 
{
  // get the system call
  int * esp = f->esp;

  struct thread *cur = thread_current();

  bad_pointer(esp); // check 

  if(esp==NULL)
    thread_exit(); 

  sema_down(&(cur->syscall_block));
  
  //check for other null pointers or unmapped things
  int sys_num = *esp;

  // redirect to the correct function handler
  switch(sys_num)
  { 
    case SYS_WAIT:    //3
      bad_pointer(esp+1);

      f->eax = wait (*(esp+1)); //gets first argument that is pid

      break;

    case SYS_EXEC:  //2
      bad_pointer(*(esp+1));

      f->eax = exec(*(esp+1));  //cmd_line pointer

      break;

    case SYS_HALT:     //0

      halt();
      break;

    case SYS_EXIT:    //1
      bad_pointer(esp+1);

      exit(*(esp+1));  // status
      break;

    case SYS_CREATE:   // 4

      bad_pointer(*(esp+1));
      bad_pointer(esp+2);

      f->eax = create(*(esp+1), *(esp+2));  //file char pointer and inital size

      break;

    case SYS_REMOVE:    //5

       bad_pointer(*(esp+1));

       f->eax = remove(*(esp+1));  //file char pointer

      break;

    case SYS_OPEN:     //6

      bad_pointer(*(esp+1));   

      f->eax = open(*(esp+1));  //file char pointer

      break;

    case SYS_FILESIZE:  //7

      bad_pointer(esp+1);

      f->eax = filesize(*(esp+1));  // fd index

      break;

    case SYS_READ:   

      bad_pointer(esp+1);
      bad_pointer(*(esp+2));
      bad_pointer(esp+3);

      f->eax = read(*(esp+1), *(esp+2), *(esp+3));  // fd, buffer pointer, size

      break;

    case SYS_WRITE:   
      bad_pointer(esp+1);
      bad_pointer(*(esp+2));
      bad_pointer(esp+3);

      f->eax = write(*(esp+1), *(esp+2), *(esp+3)); // fd, buffer pointer, size

      break;
    
    case SYS_SEEK:
      bad_pointer(esp+1);
      bad_pointer(esp+2);

      seek (*(esp+1), *(esp+2));

      break;

    case SYS_TELL:
      bad_pointer(esp+1);

      f->eax = tell(*(esp+1)); 

      break;

    case SYS_CLOSE:
      bad_pointer(esp+1);

      close (*(esp+1));

      break;

		default:  
     sema_up(&(thread_current()->syscall_block));
			thread_exit();
			break;
	}	
  sema_up(&(thread_current()->syscall_block));  //one syscall at a time
}

void
close_files()
{
  struct thread *cur = thread_current();

  int counter;
  for(counter = 2; counter < 130; counter++)
    close(counter);
}

// helper to check if pointer is mapped, !null and in the user address space
void
bad_pointer(int *esp) 
{
  struct thread *cur = thread_current();
  
  // check if esp is a valid ptr at all
  if(esp == NULL) //need to check for unmapped
  {
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
    thread_exit();
  }

  // make sure esp is in the user address space
  if(is_kernel_vaddr(esp))
  {
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
    thread_exit();
  }
    
  // check if esp is unmapped
  if(pagedir_get_page(cur->pagedir, esp) == NULL)
  {
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
    thread_exit();
  }
}

void 
halt (void)
{
  shutdown_power_off();
} 
 
void  
exit (int status)
{
  struct thread *cur = thread_current();

  cur->exit_status = status;

  char *save_ptr; // for spliter  

  printf ("%s: exit(%d)\n", strtok_r(cur->name, " ", &save_ptr), status);

  file_close (cur->save); // closing file so that write can be allowed

  close_files();
  thread_exit();
}
  
// execute the given command line
int 
exec (const char *cmd_line)
{
  struct thread *cur = thread_current();

  // get string of args and file name, pass into execute
  int tid = process_execute(cmd_line);

  // wait for child to return
  if(tid == -1) // if not loaded correctly
    return -1;
  else    // if child loaded correctly
    return tid;      
}

int 
wait (pid_t pid)
{
  return process_wait (pid);  // call process_wait

  // return return_val;                    // finished waiting
}

bool 
create (const char *file, unsigned initial_size)
{
  return filesys_create(file, initial_size);
}

bool 
remove (const char *file)
{
  return filesys_remove(file);
}

int 
open (const char *file)
{
  struct thread *cur = thread_current();
  cur->file_pointers[cur->fd_index] = filesys_open(file);

  if(cur->file_pointers[cur->fd_index] == NULL)
    return -1;
  else
    return cur->fd_index++;
}

int 
filesize (int fd)
{
  // Make sure that the file is valid
  if(fd<2 || fd>thread_current()->fd_index)
    return 0;

  return file_length (thread_current()->file_pointers[fd]);
}

int 
read (int fd, void *buffer, unsigned size)
{  
  struct thread *cur = thread_current();
  
  // read from keyboard
  if(fd == 0)
  {
    int i;
    for(i = 0; i < size; i++)
    {
      // cast to char
      char * c_ptr = (char *) buffer;
      *(c_ptr+i) = input_getc();
    }
    return size;
  }

  // if fd is valid, read the file  
  if(fd > 1 && fd <= cur->fd_index && buffer != NULL && size >= 0)
    return file_read(cur->file_pointers[fd], buffer, size);

  return -1;
}

int 
write (int fd, const void *buffer, unsigned size)
{
  struct thread * cur = thread_current();

  if(fd > 0 && fd <= cur->fd_index && buffer != NULL && size >= 0)
  {
    sema_down(&sema_write);

    unsigned size_cpy = size;
    //write to console
    if(fd==1)
    {
      // if there is nothing to write
      if(size == 0 || buffer == NULL)
      {
        sema_up(&sema_write);
        return 0;
      }    
      putbuf(buffer, size);

      sema_up(&sema_write);
      return size;
    }
    if(fd != 0 && fd != 1)  //writing to file
    {
      sema_up(&sema_write);
      struct thread *cur = thread_current();

      int k = file_write (cur->file_pointers[fd], buffer, size);

      return k;
    }

    // let others write now
    sema_up(&sema_write);
   
    return size;
  }
  // bad fd
  return 0;
}


void 
seek (int fd , unsigned position)
{
  if((fd>=2 && fd<=thread_current()->fd_index) || position>=0)
    file_seek (thread_current()->file_pointers[fd], position);
  else
  {
    sema_up(&(thread_current()->syscall_block));
    thread_exit();
  }
}

unsigned 
tell (int fd)
{
  if(fd<2 || fd>thread_current()->fd_index)
  {
    sema_up(&(thread_current()->syscall_block));
    thread_exit();
  }

  return file_tell (thread_current()->file_pointers[fd]);
}

void 
close (int fd)
{
  struct thread * t = thread_current();    // get the current thread
  
  if(fd>=2 && fd<=t->fd_index)          // check fd is valid
    if(t->file_pointers[fd] != NULL)        // check that it is not closed
    { 
      file_close (t->file_pointers[fd]);  // close the file
     
      // keep closed file from closing
      t->file_pointers[fd] = NULL;      
    }
  // else fail silently
}
