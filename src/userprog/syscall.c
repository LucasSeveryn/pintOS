#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

/* Reads a byte at user virtual address UADDR. 
UADDR must be below PHYS_BASE.
Returns the byte value if successful, -1 if a segfault occurred. */
static int
get_user (const uint8_t *uaddr)
{
	int result;
	if (!((void *) uaddr < PHYS_BASE))
	{
		printf ("Trying to access memory address: %p, which is kernel memory address\n", uaddr);
		return -1;
	}
	asm ("movl $1f, %0; movzbl %1, %0; 1:"
		: "=&a" (result) : "m" (*uaddr));
	return result;
}

/* Writes BYTE to user address UDST. UDST must be below PHYS_BASE.
Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
	int error_code;
	if (!((void *) udst < PHYS_BASE)) 
	{
		printf ("Trying to write to memory address: %p, value %u.\n",
				 udst, byte);
		return false;
	}
	asm ("movl $1f, %0; movb %b2, %1; 1:"
		: "=&a" (error_code), "=m" (*udst) : "q" (byte));
	return error_code != -1;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  //syscall_noa[ SYS_EXIT ] = 0;

}

static void
syscall_handler (struct intr_frame *f) 
{
  printf ("system call!\n");

  //void * args = syscall_retrieve_args(f);
  //int noa = (int) args[0];
	//syscall_functions( noa );
  
  thread_exit ();
}
/*
static void *
syscall_retrieve_args(struct intr_frame *f){
	void * args = new (void*)[3];
	
	return args;
} 

static int 
exit( void * args ){
	
}*/
