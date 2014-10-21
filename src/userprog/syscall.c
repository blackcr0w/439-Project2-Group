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
  //sema_init(&sema_pwait, 1);
  sema_init(&sema_write, 1);
//printf("\n\n\nprintout\n\n\n");
}

static void
syscall_handler (struct intr_frame *f) 
{
  //printf ("system call!\n");

  // get the system call
  int * esp = f->esp;

  int * ptr = *esp;
  bad_pointer(esp);

  // bad_pointer(esp);
  if(esp==NULL)
    thread_exit();
// printf("esp is...\n%p\n\n\n", *esp);


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
      bad_pointer(*(esp+1));
      cmd_line = *(esp+1); // check buffer
      f->eax = exec(cmd_line);
      break;

    case SYS_HALT:     //0

      halt();
      break;

    case SYS_EXIT:    //1

      bad_pointer(esp+1);

      status = *(esp+1); // gets first argument
      exit(status);
      break;

    case SYS_CREATE:   // 4

      bad_pointer(*(esp+1));
      bad_pointer(esp+2);

      file = *(esp+1);
      initial_size = *(esp+2);
     // printf("file name is: %s\n initial size is: %d\n", file, initial_size);
      f->eax = create(file, initial_size);

     // eax = create(file, initial_size);  //boolean value
      break;

    case SYS_REMOVE:    //5

     bad_pointer(*(esp+1));

      file = *(esp+1);
      f->eax = remove(file);
      break;

    case SYS_OPEN:     //6

      // bad_pointer(esp+1);

    bad_pointer(*(esp+1));

      file = *(esp+1);

     // printf("Open file: %s", file); // write();
      f->eax = open(file);


      break;

    case SYS_FILESIZE:  //7

    //  bad_pointer(esp+1);

      fd = *(esp+1);
      f->eax = filesize(fd);
      break;

    case SYS_READ:   
      bad_pointer(esp+1);
      bad_pointer(*(esp+2));
      bad_pointer(esp+3);

      fd = *(esp+1);
      buf = *(esp+2);
      size = *(esp+3); 
      f->eax = read(fd, buf, size);
      break;

    case SYS_WRITE:   
      bad_pointer(esp+1);
      bad_pointer(*(esp+2));
      bad_pointer(esp+3);
      
      fd = *(esp+1);
      buf = *(esp+2);
      size = *(esp+3); 

      f->eax = write(fd, buf, size);

    //  thread_exit ();
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
			thread_exit();
			break;
	}
	
}

// helper to check if pointer is mapped, !null and in the user address space
void
bad_pointer(int *esp) 
{
// printf("check if %p is a bad pointer...\n\n\n\n", esp);
  // if(*esp == 0x20101234)
    // ASSERT(0);
    // printf("i\n\n\n\n got here");

  struct thread *cur = thread_current();
  
  // check if esp is a valid ptr at all
  if(esp == NULL) //need to check for unmapped
  {
    // printf("\n\n\nBAAAD SPOT1\n\n\n");
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
    thread_exit();
  }

  // make sure esp is in the user address space
  if(is_kernel_vaddr(esp))
  {
    // printf("\n\n\nBAAAD SPOT2\n\n\n");
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
    thread_exit();
  }
    
  // check if esp is unmapped
  if(pagedir_get_page(cur->pagedir, esp) == NULL)
  {
    // printf("\n\n\nBAAAD SPOT3\n\n\n");
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
    thread_exit();
  }



 /* if(is_kernel_vaddr(*esp))
  {
    printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
     //printf("\n\n\nBAAAD SPOT2\n\n\n");
    thread_exit();
  }*/

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

  // printf("GOT TO exit");
  // printf("\n\nNAME: %s\n\n", cur->name);
  printf ("%s: exit(%d)\n", strtok_r(cur->name, " ", &save_ptr), status); 
  thread_exit();
}
  
// execute the given command line
int 
exec (const char *cmd_line)
{ //printf("\n\nexec cmd_line: %s\n\n\n\n", cmd_line);

  // how to actually do the if statement? How to tell if the process exists?
  /*if(strcmp(cmd_line, "no-such-file")==0) {
    // printf("heyyyyyyyyy\n\n\n"); 
    return -1;
  }*/
  //printf("\n\n\n\n\nEXEC IS HERE\n\n\n\n");
  struct thread *cur = thread_current();

  // get string of args and file name, pass into execute
  int tid = process_execute(cmd_line);

  // wait for child to return
  if(tid) // if successfully waited
    return tid;
  else    // if child erred on wait
    return -1;  
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
  cur->file_pointers[cur->fd_index] = filesys_open(file);

  if(cur->file_pointers[cur->fd_index] == NULL)
    return -1;
  else
    return cur->fd_index++;
}

int 
filesize (int fd)
{ //printf("fizesize fd: %d\n\n\n", fd);
  // Make sure that the file is valid
  if(fd<2 || fd>thread_current()->fd_index)
    return 0;

  return file_length (thread_current()->file_pointers[fd]);
}

int 
read (int fd, void *buffer, unsigned size)
{
  // printf("read fd: %d\n\n\n", fd);
  
  struct thread *cur = thread_current();
  //sema_down(&sema_write);
  //printf("\nsize: %d\n", size); 
  //error checking
  //char * reading = NULL;
  
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

  //sema_up(&sema_write);
  return -1;
}

int 
write (int fd, const void *buffer, unsigned size)
{

 // printf("We got to write!\n\n\n\n");

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
  // bad fd
  return 0;
}


void 
seek (int fd , unsigned position)
{
  if((fd>=2 && fd<=thread_current()->fd_index) || position>=0)
    file_seek (thread_current()->file_pointers[fd], position);
  else
    thread_exit();
}

unsigned 
tell (int fd)
{
  if(fd<2 || fd>thread_current()->fd_index)
    thread_exit();

  return file_tell (thread_current()->file_pointers[fd]);
}

void 
close (int fd)
{
  struct thread * t = thread_current();    // get the current thread
  
  // printf("fd: %d\nfd index: %d\n\n\n", fd, t->fd_index);
  
    if(fd>=2 && fd<=t->fd_index)          // check fd is valid
  if(t->file_pointers[fd] != NULL)        // check that it is not closed
    { 
      // printf("did I get here...\n\n\n");
      file_close (t->file_pointers[fd]);  // close the file
     
      // keep closed file from closing
      t->file_pointers[fd] = NULL;
    }
  // else fail silently
}
