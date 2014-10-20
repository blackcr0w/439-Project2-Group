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
struct semaphore sema_pwait;
struct semaphore sema_write;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  sema_init(&sema_exec, 1);
  sema_init(&sema_pwait, 1);
  sema_init(&sema_write, 1);
//printf("\n\n\nprintout\n\n\n");
}

static void
syscall_handler (struct intr_frame *f) 
{
  //printf ("system call!\n");

  // get the system call
  int * esp = f->esp;

  bad_pointer(esp);


 int fd;
 int pid;
 char * cmd_line;
 int status;
 char * file;
 char * buf;
 unsigned int size;
 unsigned int initial_size;

  
  //check for other null pointers or unmapped things
  int sys_num = *esp;


	// redirect to the correct function handler
	switch(sys_num)
	{ 
		case SYS_WAIT:    //3

      bad_pointer(esp+1);
      pid = *(esp+1); //gets first argument that is pid
      f->eax = wait (pid);
			break;

    case SYS_EXEC:  //2

      bad_pointer(esp+1);
      cmd_line = *(esp+1); // gets first argument
      f->eax = exec(cmd_line);
      break;

    case SYS_HALT:     //0

      halt();
      break;

    case SYS_EXIT:    //1

    //  bad_pointer(esp+1);

      status = *(esp+1); // gets first argument
      exit(status);
      break;

    case SYS_CREATE:   // 4

      bad_pointer(esp+1);
      bad_pointer(esp+2);

      file = *(esp+1);
      initial_size = *(esp+2);
     // printf("file name is: %s\n initial size is: %d\n", file, initial_size);
      f->eax = create(file, initial_size);

     // eax = create(file, initial_size);  //boolean value
      break;

    case SYS_REMOVE:    //5

    //  bad_pointer(esp+1);

      file = *(esp+1);
      //eax = remove(file);               //boolean value
      break;

    case SYS_OPEN:     //6

    //  bad_pointer(esp+1);

      file = *(esp+1);

     // printf("Open file: %s", file); // write();
      f->eax = open(file);       //boolean value


      break;

    case SYS_FILESIZE:  //7

    //  bad_pointer(esp+1);

      fd = *(esp+1);
      filesize(fd);
      break;

    case SYS_READ:   

      // bad_pointer(esp+1);
      // bad_pointer(esp+2);
      // bad_pointer(esp+3);

      fd = *(esp+1);
      buf = *(esp+2);
      size = *(esp+3); 
      f->eax = read(fd, buf, size);
      break;

    case SYS_WRITE:   

      bad_pointer(esp+1);
      bad_pointer(esp+2);
      bad_pointer(esp+3);

      fd = *(esp+1);
      buf = *(esp+2);
      size = *(esp+3); 

   //  printf("FD should be 1: %d", fd); // write();
    //  printf("Buffer is: %s", buf); // write();
     // printf("size is: %d", size); // write();

      f->eax = write(fd, buf, size);

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

void
bad_pointer(int *esp)
{
  struct thread *cur = thread_current();
 // printf("ESP is: %p\n", esp);
  if(esp == NULL) //need to check for unmapped
  {
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);

    //  printf("\n\n\nBAAAD SPOT1\n\n\n");

    thread_exit();
  }
      
  if(pagedir_get_page(cur->pagedir, esp) == NULL || is_kernel_vaddr(esp))
  {
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
     //printf("\n\n\nBAAAD SPOT2\n\n\n");
    thread_exit();
  }

  /*if(*esp == NULL)
  {
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
     printf("\n\n\nBAAAD SPOT2\n\n\n");
    thread_exit();
  }*/
 
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

  cur->parent->exit_status = status;

  char *save_ptr; // for spliter
  // token = strtok_r (cur->name, " ", &save_ptr);

  //  printf("GOT TO exit");
  // printf("\n\nNAME: %s\n\n", cur->name);
  printf ("%s: exit(%d)\n",strtok_r (cur->name, " ", &save_ptr), status); 
  thread_exit();
}
 
// execute the given command line
int 
exec (const char *cmd_line)
{

  //printf("\n\n\n\n\nEXEC IS HERE\n\n\n\n");
  struct thread *cur = thread_current();


  char s[100];
  strlcpy (s, cmd_line, strlen(cmd_line)+1);  //putting cmdline in s

  char *save_ptr; // for spliter
  char * file_name = strtok_r (s, " ", &save_ptr);

  // get string of args and file name, pass into execute
  int tid = process_execute(file_name);

  sema_down(&(cur->exec_block));

  // wait for child to return
  if(tid) // if successfully waited
  {
    // return eax
    return tid;
  }
  else        // if child erred on wait
  {
    return -1;
  }  
}

int 
wait (pid_t pid)
{
  //printf("before wait\n");
  //sema_down(&sema_pwait);                // block any duplicate waits
  int return_val = process_wait (pid);  // call process_wait
 //  printf("after wait\n");                // finished waiting

  return return_val;                    // finished waiting
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
   // cur->fd_index = (cur->fd_index) + 1;
    cur->file_pointers[cur->fd_index] = filesys_open(file);

  //   p rintf("File desctriptor Just added: %d\n", cur->file_pointers[cur->fd_index]);
    if(cur->file_pointers[cur->fd_index] == NULL)
    {
      return -1;
    }
    else
    {

      return cur->fd_index++;
    }
    
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
  //printf("\nsize: %d\n", size);
  //error checking
  //char * reading = NULL;
 /* if(fd ==0) //read from keyboard 
  {
    int i;
    for(i = 0; i < size; i++)
    {
  //    *(buffer+i) = input_getc();
    }
    return size;
  }*/
  if(fd > 0 && fd <= cur->fd_index && buffer != NULL && size >= 0)
  {
    int ret = file_read(cur->file_pointers[fd], buffer, size);

    printf("\nfd: %d\n", fd);
   // printf("\nfinished size: %d\n", ret);
    return ret;
  }
  return -1;
}

int 
write (int fd, const void *buffer, unsigned size)
{

 // if(fd < 0)
 //   thread_exit();
 // printf("We got to write!");
 // printf("File desctriptor at start of write: %d\n", fd);

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

    //if(size<=200)
    //{
      putbuf(buffer, size);

      sema_up(&sema_write);
      return size;
   // }

   /* int i;
    for(i = 0; i<size; i+=200)
    {
      if(size_cpy <= 200)
      {
        putbuf(buffer + i, size_cpy);
      }

      putbuf(buffer + i, 200);

      size_cpy = size_cpy - 200;
     
    }*/

  }
  if(fd != 0 && fd != 1)
  {
    //writing to file
    sema_up(&sema_write);
    struct thread *cur = thread_current();
    //printf("File desctriptor is: %d\n", fd);
    return file_write (cur->file_pointers[fd], buffer, size);

  }

  // let others write now
  sema_up(&sema_write);

  return size;
}


void 
seek (int fd , unsigned position)
{
  if((fd>=2 && fd<=thread_current()->fd_index) || position>=0)
    file_seek (thread_current()->file_pointers[fd], position);
}

unsigned 
tell (int fd)
{
  if(fd<2 || fd>thread_current()->fd_index)
    return 0;

  return file_tell (thread_current()->file_pointers[fd]); // maybe plus 1
}

void 
close (int fd)
{
  if(fd>=2 && fd<=thread_current()->fd_index)
    file_close (thread_current()->file_pointers[fd]); 
}
