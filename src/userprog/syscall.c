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
	asm ("movl $1f, %0; movb %b2, %1; 1:"
		: "=&a" (error_code), "=m" (*udst) : "q" (byte));
	return error_code != -1;
}

// USE THIS SHIT TO ACCESS MEMORY
/* Wrapper to retrieve address specified by user pointer */
static int
get_data_user (const uint8_t *uaddr)
{
	if ((void *) uaddr < PHYS_BASE) 
	{
		return get_user (uaddr);
	}
	else
	{
		printf ("Trying to access memory address: %p, which is kernel memory address\n", uaddr);
		return -1;
	}
}

// USE THIS SHIT TO ACCESS MEMORY
/* Wrapper to write to user address specified by user pointer */
static bool 
put_data_user (uint8_t *udst, uint8_t byte)
{
	if ((void *) udst < PHYS_BASE)
	{
		return put_user (udst, byte);
	}
	else
	{
		printf ("Trying to write to memory address: %p, value %u.\n",
				 udst, byte);
		return false;
	}
}

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
}
