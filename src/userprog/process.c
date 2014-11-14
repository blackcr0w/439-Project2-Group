#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
//#include "vm/frame.h"
#include "vm/page.h"

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
 
// Dakota and Jeff driving here   
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;
  struct thread * cur = thread_current ();

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);

  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  char s[100];
  strlcpy (s, file_name, strlen(file_name)+1);  //putting cmdline in s

  char *save_ptr; // for spliter
  char * file = strtok_r (s, " ", &save_ptr);

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file, PRI_DEFAULT, start_process, fn_copy);
  if (tid == TID_ERROR)
    palloc_free_page (fn_copy); //change to free the frame

  struct thread *child = get_thread_tid (tid);

  child->root = 0;

  if(!child == NULL)
  {
    child->parent = cur;  // if the child exists set its parent to the current thread
  }
  sema_down (&(cur->exec_block));

  if(child->load == 0)  // if load not successful
    return -1;

  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (file_name, &if_.eip, &if_.esp);

  /* If load failed, quit. */
  palloc_free_page (file_name);
  if (!success) 
  {
    thread_current ()->load = 0;
    thread_exit ();
  }


  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}  

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */

 // Cohen, Spencer, Jeff, and Dakota driving here  
int
process_wait (tid_t child_tid) 
{
  struct thread *current =  thread_current ();
 
  // loop through the children of currently running thread
  if(list_empty(&current -> children))
  {
    return -1;
  } 

  struct list_elem *current_child;  // list elem for loop
  for (current_child = list_begin (&current -> children); current_child != list_end (&current -> children);
            current_child = list_next (current_child))
  {
    struct thread *t = list_entry (current_child, struct thread, child_elem);
    tid_t tid = t -> tid;
    if(tid == child_tid) // if found direct child
    {
      list_remove (&t->child_elem);
      
      sema_down (&current->sema_parent_block);   // block parent so that child may finish
      int stat = t->exit_status;                // get status of child before they exit
      sema_up (&t-> wait_block);                 // unblocking so child can finish
  
      return stat;
    }
  }
    return -1;
} 
 
/* Free the current process's resources. */

//Spencer and Cohen driving here
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  //setting all parents to NULL
  struct list_elem *set_null;
  for (set_null = list_begin (&cur->children); set_null != list_end (&cur->children);
          set_null = list_next (set_null))
  {
    struct thread *child = list_entry (set_null, struct thread, child_elem);
    child->parent = NULL;
  } 

  // WHY DOES IT HANG ON THE SEMAPHORE WITHOUT THE CHECK
  // WHY DOES IT HANG ON THE PAGEDIR WITH THE CHECK
  if(cur->parent != NULL /*&& !cur->parent->root*/) // if parent not dead tell them that this thread is about to die
  {
    // printf("\nhi %d\n", cur->parent->root);

    sema_up (&cur-> parent -> sema_parent_block);
    sema_down (&cur-> wait_block); // blocking so parent can get info first
    // printf("\n\n\n\n\nPROCESS EXIT\n");
    // MAY HAVE ISSUE WITH ROOT, MAYBE USE GLOBAL VARIABLE
  }
  //clear_thread_frames();

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL) 
    {
    // printf("\nhi %s\n", cur->name);
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp, char * file_name);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;



  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  init_page_table();
  
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  //Dakota driving here

  char *save_ptr; // for spliter

  char s[100]; //setting up s

  strlcpy (s, file_name, strlen (file_name)+1);  //putting cmdline in s

  char * real_file_name = strtok_r (s, " ", &save_ptr);  

  /* Open executable file. */
  file = filesys_open (real_file_name); //fix

  thread_current ()->save = file;

  if (file == NULL || file==0) 
    {
      printf ("load: %s: open failed\n", real_file_name);     
      goto done;  
    }

  file_deny_write(file);

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", real_file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        } 
    }   

  /* Set up stack. */
  if (!setup_stack (esp, file_name))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;
  // printf("\nlookup is NULL in load %p\n", eip);

  success = true;

  done:
  /* We arrive here whether the load is successful or not. */
  sema_up(&t->parent->exec_block);  //sema up on parents blocking on exec semaphore

  return success;
}

/* load() helpers. */

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user proce
   ss if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */

static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);
 
     // init_page_table ();
  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    { 
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
    //  uint8_t *kpage = palloc_get_page (PAL_USER);

      //update supplemental

  /*printf("\nFile in load is: %p\n", file);
  printf("ReadBytes in load is: %d\n", page_read_bytes);
  printf("ofs in load is: %d\n", ofs);*/


 
      struct page *p = malloc (sizeof (struct page)); //make a new page

      p -> access = 0;

      p -> file = file;
      p -> ofs = ofs;
      p -> VA = (void*) upage;
      p -> page_read_bytes = page_read_bytes;
      p -> page_zero_bytes = page_zero_bytes;
      p -> writable = writable;

//printf("Load Segment\n");
//printf("file offset %d\n", ofs);
    /* printf("File in load is: %p\n", p->file);
  printf("ReadBytes in load is: %d\n", p->page_read_bytes);
  printf("ofs in load is: %d\n", p->ofs);*/

//printf("lookup is %p\n\n", p->VA);
//printf("pages is %p\n", file);

 //printf("\nStill Going2\n");
      insert_page (p);
      //if(page_lookup (p->VA) == NULL)
     //   printf("\nlookup is NULL %p\n", page_lookup (p->VA) -> VA);

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
      ofs += page_read_bytes;

    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */

// Spencer and Dakota driving for this method
static bool
setup_stack (void **esp, char *file_name) 
{
  uint8_t *kpage;
  bool success = false;

  struct page *p = malloc (sizeof (struct page)); //make a new page
  p->VA = ((uint8_t *) PHYS_BASE) - PGSIZE;
  p->dirty = 0;

  //insert_page(p);


  thread_current ()-> stack_bottom = p-> VA;
//printf("\nStack bottom is %p\n", p->VA);

 // kpage = palloc_get_page (PAL_USER | PAL_ZERO);

 // worry about stack growth later
 kpage = get_new_frame (p);
  
  ASSERT(kpage != NULL) // get_new_frame should handle eviction if necessary
  
  success = install_page ( ((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true );
  if (success)
    *esp = PHYS_BASE; // temporary 
  else
    palloc_free_page (kpage);

  // our code starts
  char *myEsp = (char*) *esp;
  char *file_cpy = file_name;   // copying cmdline
  char *token, *save_ptr;       // for spliter
  char s[strlen (file_cpy)];

  strlcpy(s, file_cpy, strlen (file_cpy)+1);  //moves cmdline copy into s, add 1 for null

  int count = -1;  //tracking number of arguments 

  // limiting number of characters on stack to 130
  char *args[130] = {NULL};
  char *whereArgs[130] = {NULL};

  for (token = strtok_r (s, " ", &save_ptr); token != NULL;
        token = strtok_r (NULL, " ", &save_ptr))
  {
    count++; // how many arguments
    args[count] = token; // token 

    // if stack limit has been surpassed
    ASSERT(count<=130); 
  }

  int count_save = count;
  while(count >= 0)  // while still has args
  { 
    int i;

    myEsp--;
    *myEsp = NULL;  // store end nullcharacter on stack and decrement stack

    for(i = strlen (args[count]+1); i>=0; i--)  // loops through argument characters
    {
      myEsp--;
      *myEsp = args[count][i]; // store the character on stack and decrement stack
    }

    whereArgs[count] = myEsp;

    count--;  // go to next argument
  }

  int align = (int) myEsp % 4;
  align += 4;
 
  while(align != 0)  // while not word aligned
  {
    char word = NULL;
    myEsp--;
    *myEsp = word;
    align--;
  }

  int n = NULL;
  myEsp-= 4;
  *myEsp = n;  // push 0

  count = count_save;
  while(count >= 0)  // while still has args 
  {     
    myEsp -= sizeof(char *);
    
    *((char **)myEsp) = whereArgs[count];  // pointer to argument
  
    count--;  // go to next argument
  }

  char *start = myEsp;

  myEsp -= 4;
  *(char **)myEsp = start;

  myEsp -= 4; // adds number of arguments to stack
  *myEsp = count_save + 1;

  void * ret; // arbitrary return value

  myEsp -= 4;
  *myEsp = ret;  // final return adress

  *esp = myEsp;

  // hex_dump(*esp, *esp, PHYS_BASE-*esp,1);
//printf ("ESP in stack is %p\n\n", myEsp);
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}
