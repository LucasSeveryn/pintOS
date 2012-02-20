#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
/*
static int syscall_noa[ NOA ];
static int* (*syscall_functions[ NOA ]) (void*);
*/
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