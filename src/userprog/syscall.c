#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include <kernel/stdio.h>

void (*)(char* , struct intr_frame *) syscall_functions[NOA];

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
  syscall_functions[SYS_HALT] = &syscall_halt;
  syscall_functions[SYS_EXIT] = &syscall_exit;
  syscall_functions[SYS_EXEC] = &syscall_exec;
  syscall_functions[SYS_WAIT] = &syscall_wait;
  syscall_functions[SYS_WRITE] = &syscall_write;

  syscall_noa[SYS_HALT] = 0;
  syscall_noa[SYS_EXIT] = 1;
  syscall_noa[SYS_EXEC] = 1;
  syscall_noa[SYS_WAIT] = 1;
  syscall_noa[SYS_WRITE] = 3;

}

static void
syscall_handler (struct intr_frame *f) 
{
  int syscall_number = *(f -> esp - 4);
  char * args = syscall_retrieve_args(f);
  int noa = (int) args[0];
  f->eax = syscall_functions[syscall_number]( args, f );
  syscall_functions[syscall_number]( args );
}

static char *
syscall_retrieve_args(struct intr_frame *f, struct intr_frame *f){
  char * args = new (char*)[3];
  int noa = syscall_noa[ *(f -> esp - 4) ];

  for(int i = 0; i <= noa; i++){
    args[i] =  *(f -> esp - (i + 1) * 4);
  }

  return args;
} 

static void 
syscall_halt(char * args, struct intr_frame *f){
  shutdown_power_off();
}

static void 
syscall_exit(char * args, struct intr_frame *f){
  f->eax = args[1];
  thread_current() -> ret = args[1];
  thread_exit();
}

static void 
syscall_exec(char * args, struct intr_frame *f){
  struct thread * parent = thread_current();
  tid_t id = process_execute (args);
  if(id!=-1)thread_add_child (parent, id);
  f->eax = id;
}

static void 
syscall_wait( char * args, struct intr_frame *f ){
  f->eax = process_wait( args[1] );
}

static void 
syscall_write( char * args, struct intr_frame *f ){
  if(args[1] == 1){
    uint8_t buffer_addr = args[2];
    size_t size = args[3];
    char * buffer = (char *)malloc(size +1);

    int i;
    for(i = 0; i < size; i++){
      char byte = get_user(buffer_addr+i);
      buffer[i] = byte;
    }

    putbuf(buffer, size);
  }
}
