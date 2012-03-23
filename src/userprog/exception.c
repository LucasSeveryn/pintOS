#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "threads/synch.h"
#include "vm/swap.h"
#include "vm/page.h"
#include "vm/frame.h"

static bool DEBUG = false;

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
   to implement virtual memory.  Some solutions to task 2 may
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
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();

  /* Count page faults. */
  page_fault_cnt++;
  void * fault_page = (void *) (PTE_ADDR & (uint32_t) fault_addr);
  struct thread *t = thread_current ();
  char * name = t->name;
  tid_t tid = t->tid;
  if(DEBUG)printf("\n(%s - %d) Current Page Fault: %lld\n", name, tid, page_fault_cnt);
  if(DEBUG)printf("(%s - %d) We have faulted to %p |#| ", name, tid, fault_page);
  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;

  if (!is_user_vaddr(fault_addr))
    syscall_t_exit (t->name, -1);

  sema_down (t->pagedir_mod);
  void *ret_page = pagedir_get_page(t->pagedir, fault_page);
  sema_up (t->pagedir_mod);

  if(DEBUG)printf("Current mapping for faulted address is %p\n", ret_page);
  void *esp = f->cs == SEL_KCSEG ? t->esp : f->esp;

  bool stack_access = is_stack_access (esp, fault_addr);
  if (ret_page == 0 && !stack_access)
  {
    f->eip = (void (*) (void)) f->eax;
    f->eax = 0xffffffff;
    syscall_t_exit (t->name, -1);
  }

  bool writable = true;
  bool dirty = false;
  uint8_t *kpage = NULL;
  if(ret_page != 0)
  {
    struct suppl_page *page = (struct suppl_page *) ret_page;
    if(DEBUG)printf("(%s - %d) Supplemental page table is located in %d\n", name, tid, page->location);
    kpage = frame_get (fault_page, true, page->origin);
    /* Get a page of memory. */
    switch (page->location)
    {
      case EXEC:
      case FILE:
        filesys_lock_acquire ();
        file_seek (page->origin->source_file, page->origin->offset);
        int a;

        void * br = malloc (PGSIZE);
        if ((a = file_read (page->origin->source_file, br, page->origin->zero_after))
          != (int) page->origin->zero_after)
        {
          filesys_lock_release();
          lock_frames ();
          frame_free (kpage);
          unlock_frames ();
          free (br);
          syscall_t_exit (t->name, -1);
        }
        filesys_lock_release();

        frame_pin (fault_page, PGSIZE);

        memcpy (kpage, br, PGSIZE);

        frame_unpin (fault_page, PGSIZE);

        free (br);

        memset (kpage + page->origin->zero_after, 0, PGSIZE - page->origin->zero_after);
        writable = page->origin->writable;
        break;
      case SWAP:
        frame_pin (fault_page, PGSIZE);
        swap_load( kpage, page->swap_elem );
        frame_unpin (fault_page, PGSIZE);
        dirty = true;
        break;
      case ZERO:
        memset (kpage, 0, PGSIZE);
        break;
    }
    if(pg_ofs(page) != 0) {
      free(page);
    }
  }

  if (kpage == NULL) {
    kpage = frame_get (fault_page, true, NULL);
  }
  sema_down (t->pagedir_mod);
  pagedir_clear_page (t->pagedir, fault_page);
  /* Add the page to the process's address space. */
  if (!pagedir_set_page (t->pagedir, fault_page, kpage, writable))
  {
    sema_up (t->pagedir_mod);
    lock_frames ();
    frame_free (kpage);
    unlock_frames ();
    syscall_t_exit (t->name, -1);
  }
  if(DEBUG)printf("(%s - %d) At the end virtual address %p points to %p\n", name, tid, fault_page, kpage);
  pagedir_set_dirty (t->pagedir, fault_page, dirty);
  pagedir_set_accessed (t->pagedir, fault_page, true);
  sema_up (t->pagedir_mod);
}