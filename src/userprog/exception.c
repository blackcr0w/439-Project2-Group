#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "threads/palloc.h"
#include "vm/page.h"
#include "vm/swap.h"

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}
 
/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */
     
  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f) 
{
  //  printf("Page Fault\n");

  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  asm ("movl %%cr2, %0" : "=r" (fault_addr));
  // printf("faulting address: %p\n", fault_addr);

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();  

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
 
 
  printf ("Fault Address: %p\n", fault_addr);
  /* Count page faults. */
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write =       (f->error_code & PF_W) != 0;
  user =        (f->error_code & PF_U) != 0;

  int * esp = f->esp;

  void * previous_page = pg_round_up (fault_addr);
  void * round_down_va = pg_round_down(fault_addr);

  char *check_limit = PHYS_BASE + PGSIZE - MB8;

  struct page * p = page_lookup (round_down_va);

  int dif =  (int)esp -(int)fault_addr;

   
  if(fault_addr >= (int)esp - 32 && user)
  { 
    if(fault_addr < check_limit)   // if va is greater than the 8MB limit, exit 
    {
      printf ("%s: exit(%d)\n", thread_current ()->name, thread_current ()->exit_status);
      thread_exit (); 
    }
    struct page *p_stack = malloc (sizeof (struct page)); // make a new page
    void *kpage = get_new_frame (p_stack);
   
    p_stack-> writable = write;
    thread_current ()-> stack_bottom -= PGSIZE;
    p_stack-> VA = thread_current ()-> stack_bottom;
    if (!install_page (p_stack -> VA, kpage, p_stack-> writable))  // puts mapping in page directory
    {
      p_stack -> present = 0;
      p_stack -> in_frame_table = 0;
      palloc_free_page (kpage);
    }
    else
    {
      p_stack -> in_frame_table = 1;
      p_stack -> present = 1;
      insert_page (p_stack);
      return;
    }
  } 

  if(p == NULL)
  {
    printf ("%s: exit(%d)\n", thread_current ()->name, thread_current ()->exit_status); //trying to present data never asked for
    thread_exit ();
  }
  if(p -> present == 0) //if memory has not yet been allocated for this page then allocate it
  {
    void *kpage = get_new_frame (p);  // get a physical memory spot for the faulting process

    // Load this page. 
    if (file_read_at (p -> file, kpage, p -> page_read_bytes, p -> ofs) 
          != (int) p -> page_read_bytes)
    {
      p -> present = 0;
      p -> in_frame_table = 0;
      palloc_free_page (kpage); //if didn't read what happens
      thread_exit ();
    }
  
    memset (kpage + (p -> page_read_bytes), 0, p -> page_zero_bytes);

    // Add the page to the process's address space. 
    if (!install_page (p -> VA, kpage, p -> writable))     //puts mapping in page directory
    {
      p -> present = 0;
      p -> in_frame_table = 0;
      palloc_free_page (kpage);
    }
    else
    {
      p -> present = 1;
      p -> in_frame_table = 1;
      return;
    }   
  }

  else if(!p->in_frame_table)//if it is in swap
  { 
    void *kpage = get_new_frame (p);
    if (!install_page (p -> VA, kpage, p -> writable))     //puts mapping in page directory
    {
      p -> present = 0;
      p -> in_frame_table = 0;
      palloc_free_page (kpage);
    }
    else  
    {
      read_swap(p);
      insert_frame (p->frame_ptr);
      p -> in_frame_table = 1;
      p -> present = 1;
      //remove_swap (p);
      pagedir_set_dirty (thread_current ()->pagedir, p, 1); 
      return;
    }
  }


  // printf ("%s: exit(%d)\n", thread_current ()->name, thread_current ()->exit_status);
  // thread_exit ();


  /* To implement virtual memory, delete the rest of the function
     body, and replace it with code that brings in the page to
     which fault_addr refers. */
  printf ("Page fault at %p: %s error %s page in %s context.\n",
          fault_addr,
          not_present ? "not present" : "rights violation",
          write ? "writing" : "reading",
          user ? "user" : "kernel");

  printf("There is no crying in Pintos!\n");

  kill (f);
}
