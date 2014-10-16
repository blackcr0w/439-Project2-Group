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
struct semaphore * sema_exec;  // binary semaphore to control access to exec function
struct semaphore * sema_pwait;
struct semaphore * sema_write;
int *file_pointers[100] = {NULL};
int fd_index = 2;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  sema_init(&sema_exec, 1);
  sema_init(&sema_pwait, 1);
  sema_init(&sema_write, 1);
}

static void
syscall_handler (struct intr_frame *f) 
{
  printf ("system call!\n");
  thread_exit();
  //hex_dump(f->esp, f->esp, 1000,1);

  // get the system call
  int * esp = f->esp;

 // printf("The syscall is %s", *(esp+2));
  
 int fd;
 int pid;
 char * cmd_line;
 int status;
 char * file;
 char * buf;
 unsigned int size;
 unsigned int initial_size;

  if(is_kernel_vaddr(*esp))  //if the pointer is in kernal exit right away
  {
    thread_exit();
  }
  //check for other null pointers or unmapped things
  int sys_num = *esp;

  // get pointer to eax
  uint32_t * eax = f->eax;

  // get the file name and args
 // char * cmd_line = get_cmd_line((char *) f->esp);
  printf("the write call is %d\n", sys_num);
//thread_exit ();


	// redirect to the correct function handler
	switch(sys_num)
	{ 
		case SYS_WAIT:    

      pid = *(esp+1); //gets first argument that is pid
      eax = wait (pid);
			break;

    case SYS_EXEC:  

      printf("\n\n\n\n\n\n\n\n");
      cmd_line = *(esp+1); // gets first argument
      exec(cmd_line);
      break;

    case SYS_HALT:     

      halt();
      break;

    case SYS_EXIT:    

      status = *(esp+1); // gets first argument
      exit(status);
      break;

    case SYS_CREATE:   // create();

      file = *(esp+1);
      initial_size = *(esp+2);
      printf("file name is: %s\n initial size is: %d\n", file, initial_size);

      if(create(file, initial_size))
      {
        eax = 1;
        
      }
        
      else
      {
        ASSERT(0);
        eax = 0;
      }
        

     // eax = create(file, initial_size);  //boolean value
      break;

    case SYS_REMOVE:    

      file = *(esp+1);
      //eax = remove(file);               //boolean value
      break;

    case SYS_OPEN:     

      file = *(esp+1);

     // eax = open(file);       //boolean value
      break;

    case SYS_FILESIZE:  

      fd = *(esp+1);
      filesize(fd);
      break;

    case SYS_READ:   

   //   fd = *(esp+1);
  //    buf = *(esp+2);
  //    unsigned size = *(esp+3); 
  //    eax = read(fd, buf, size);
      break;

    case SYS_WRITE:   

      fd = *(esp+1);
      buf = *(esp+2);
      size = *(esp+3); 

    // printf("FD should be 1: %d", fd); // write();
    //  printf("Buffer is: %s", buf); // write();
    //  printf("size is: %d", size); // write();

      eax = write(fd, buf, size);

    //  thread_exit ();
      break;
    case SYS_SEEK:


      break;
    case SYS_TELL:
      break;
    case SYS_CLOSE:
      break;
		default: 
			printf("uh oh");
			break;
		// ...
	}
	
}

// gets the associated cmd_line from the fram
// look into the esp stack to get name plus args in correct order
/*char * get_cmd_line(char * esp UNUSED)
{
  // save esp to revert later
  char * save_esp = esp;

  // get to top of argc (reads downwards)
  esp+=4;

  // save the count of arguments
  int arg_count = *esp;

  // gets to top of arg0
  esp+=8;

  // gets where to jump into the args on top of stack
  esp = *(char**)esp;     

  // the command line to add to (for return)
  char cmd_line[1000] = {NULL};
  int i = 0;

  // get each argument and file name
  for(i = 0; arg_count > 0; i++)
  {  
    if(*esp == NULL)
    {
      *(cmd_line + i) = ' ';

      arg_count--;
    }
    else
    {
       *(cmd_line + i) = *esp;
    }

    // get the next byte
    esp++;
  }

  // revert esp to original value
  esp = save_esp;

  return cmd_line;
}*/


void 
halt (void)
{
  shutdown_power_off();
}

void 
exit (int status)
{
  struct thread *cur = thread_current();

  cur->parent->exit_status = status;
}

// execute the given command line
int 
exec (const char *cmd_line)
{
  return 0;
/*  // synchronization, only one can use at a time
  sema_down(sema_exec);

  // get string of args and file name, pass into execute
  int tid = process_execute(cmd_line);

  // wait for child to return
  if(tid) // if successfully waited
  {
    // return eax
    *eax = tid;
    return tid;
  }
  else        // if child erred on wait
  {
    // return eax
    *eax = -1;
    return -1;
  }
  sema_up(sema_exec);*/
}

int 
wait (pid_t pid)
{
  sema_down(&sema_pwait);                // block any duplicate waits
  int return_val = process_wait (pid);  // call process_wait
  sema_up(&sema_pwait);                  // finished waiting

  return return_val;                    // finished waiting
}

bool 
create (const char *file, unsigned initial_size)
{
  //char file
  //printf("file name is: %s\n initial size is: %d", initial_size);
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

    file_pointers[fd_index] = filesys_open(file);
    if(file_pointers[fd_index] == NULL)
    {
      return -1;
    }
    else
    {
      return fd_index++;
    }
    
}

int 
filesize (int fd)
{
  return 0;
}

/*int 
read (int fd, void *buffer, unsigned size)
{
  if(fd ==0) //read from keyboard 
  {
    input_getc();
  }
}*/

int 
write (int fd, const void *buffer, unsigned size)
{
 // printf("We got to write!");

 // prevent processes from writing mixed up
  sema_down(&sema_write);

  unsigned size_cpy = size;
//ASSERT(false);
  //write to console
  if(fd==1)
  {
    // if there is nothing to write
    if(size == 0 || buffer == NULL)
    {
      sema_up(&sema_write);
      return 0;
    }    

    if(size<=200)
    {
      putbuf(buffer, size);

      sema_up(&sema_write);
      return size;
    }

    int i;
    for(i = 0; i<size; i+=200)
    {
      if(size_cpy <= 200)
      {
        putbuf(buffer + i, size_cpy);
      }

      putbuf(buffer + i, 200);

      size_cpy = size_cpy - 200;
     
    }

  }
  if(fd != 0 || fd != 1)
  {
    //writing to file
    sema_up(&sema_write);
    return file_write (file_pointers[fd], buffer, size);

  }

  // let others write now
  sema_up(&sema_write);

  return size;
}


/*void 
seek (int fd , unsigned position)
{
  
}*/

/*unsigned 
tell (int fd UNUSED)
{
  return 0;
}*/

/*void 
close (int fd UNUSED)
{

}*/
