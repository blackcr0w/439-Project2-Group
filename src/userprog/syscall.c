#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "threads/synch.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
int exec (const char *cmd_line, uint32_t *eax);
// struct semaphore * sema_exec;  // binary semaphore to control access to exec function
struct semaphore * sema_pwait;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  // sema_init(sema_exec, 1);
  sema_init(sema_pwait, 1);
}

static void
syscall_handler (struct intr_frame *f) 
{
  printf ("system call!\n");
  thread_exit ();

  // get the system call
	/*int * esp = f->esp;
  int sys_num = *esp;

  // get pointer to eax
  uint32_t * eax = &(f->eax);

  // get the file name and args
  char * cmd_line = get_cmd_line((char *) f->esp);

	// redirect to the correct function handler
	switch(sys_num)
	{ 
		case SYS_WAIT:      // wait();
			break;
    case SYS_EXEC:      exec(cmd_line, eax);
      break;
    case SYS_HALT:      halt();
      break;
    case SYS_EXIT:
      break;
    case SYS_CREATE:
      break;
    case SYS_REMOVE:
      break;
    case SYS_OPEN:
      break;
    case SYS_FILESIZE:
      break;
    case SYS_READ:
      break;
    case SYS_WRITE:
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
	}*/
	
}

// gets the associated cmd_line from the fram
// look into the esp stack to get name plus args in correct order
char * get_cmd_line(char * esp UNUSED)
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
}


void 
halt (void)
{
  shutdown_power_off();
}

/*void 
exit (int status UNUSED)
{

}*/

// execute the given command line
int 
exec (const char *cmd_line UNUSED, uint32_t *eax UNUSED)
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
wait (pid_t pid UNUSED)
{
  return 0;
 /* sema_down(sema_pwait);                // block any duplicate waits
  int return_val = process_wait (pid);  // call process_wait
  sema_up(sema_pwait);                  // finished waiting

  return return_val;                    // finished waiting*/
}

/*bool 
create (const char *file UNUSED, unsigned initial_size UNUSED)
{

}*/

/*bool 
remove (const char *file UNUSED)
{

}*/

int 
open (const char *file UNUSED)
{

  return 0;
}

int 
filesize (int fd UNUSED)
{
  return 0;
}

/*int 
read (int fd, void *buffer UNUSED, unsigned size UNUSED)
{

}*/

int 
write (int fd UNUSED, const void *buffer UNUSED, unsigned size UNUSED)
{
  return 0;
}

void 
seek (int fd UNUSED, unsigned position UNUSED)
{
  
}

unsigned 
tell (int fd UNUSED)
{
  return 0;
}

/*void 
close (int fd UNUSED)
{

}*/
