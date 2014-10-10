#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();

  while(f->eip != null)
  {

	  // get the error code (which system call)
  	uint32_t error_code = f->error_code;

  	// redirect to the correct function handler
  	switch(error_code)
  	{
  		case wait: wait();
  			break;
  		case exec: exec();
  			break;
  		default:
  			printf("uh oh");
  			break;
  		// ...
  	}
  	
  }

}

void
wait()
{
	// handle wait
}