#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include <kernel/stdio.h>

void (*)(char* , struct intr_frame *) syscall_functions[NOA];

static void syscall_handler (struct intr_frame *);
static struct lock filesys_lock;

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
filesys_lock_release (){
  lock_release (filesys_lock);
};

void
filesys_lock_acquire (){
  lock_acquire (filesys_lock);
};

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

  lock_init (&filesys_lock);

  syscall_functions[SYS_HALT] = &syscall_halt;
  syscall_functions[SYS_EXIT] = &syscall_exit;
  syscall_functions[SYS_EXEC] = &syscall_exec;
  syscall_functions[SYS_WAIT] = &syscall_wait;
  syscall_functions[SYS_CREATE] = &syscall_create;
  syscall_functions[SYS_REMOVE] = &syscall_remove;
  syscall_functions[SYS_OPEN] = &syscall_open;
  syscall_functions[SYS_FILESIZE] = &syscall_filesize;
  syscall_functions[SYS_READ] = &syscall_read;
  syscall_functions[SYS_WRITE] = &syscall_write;
  syscall_functions[SYS_SEEK] = &syscall_seek;
  syscall_functions[SYS_SEEK] = &syscall_tell;
  syscall_functions[SYS_SEEK] = &syscall_close;

  syscall_noa[SYS_HALT] = 0;
  syscall_noa[SYS_EXIT] = 1;
  syscall_noa[SYS_EXEC] = 1;
  syscall_noa[SYS_WAIT] = 1;
  syscall_noa[SYS_CREATE] = 2;
  syscall_noa[SYS_REMOVE] = 1;
  syscall_noa[SYS_OPEN] = 1;
  syscall_noa[SYS_FILESIZE] = 1;
  syscall_noa[SYS_READ] = 3;
  syscall_noa[SYS_WRITE] = 3;
  syscall_noa[SYS_SEEK] = 2;
  syscall_noa[SYS_TELL] = 1;
  syscall_noa[SYS_CLOSE] = 1;
    
}

static void
syscall_handler (struct intr_frame *f) 
{
  int syscall_number = *(f -> esp - 4);
  char * args = syscall_retrieve_args (f);
  int noa = (int) args[0];
  syscall_functions[syscall_number]( args, f );
}

static void
syscall_t_exit ( char * p_name, int status){
  printf ("%s: exit(%d)\n", p_name, status);
  thread_exit ();
}

static char *
syscall_retrieve_args(struct intr_frame *f, struct intr_frame *f){
  char * args = new (char*)[3];
  int noa = syscall_noa[ get_user (f -> esp - 4) ];
  if(noa == -1){
   syscall_t_exit (thread_current () -> name, -1);
  }

  for(int i = 0; i <= noa; i++){
    args[i] = get_user (f -> esp - (i + 1) * 4);
    if(args[i] == -1) {
      syscall_t_exit (thread_current () -> name, -1);
    }
  }

  return args;
} 

static void 
syscall_halt(char * args, struct intr_frame *f){
  shutdown_power_off ();
}

static void 
syscall_exit(char * args, struct intr_frame *f){
  bool put_status = put_user (f->eax, args[1]);
  thread_current () -> ret = args[1];
  syscall_t_exit (thread_current () -> name, args[1]);
}

static void 
syscall_exec(char * args, struct intr_frame *f){
  struct thread * parent = thread_current();
  tid_t id = process_execute (args);
  if(id!=-1) thread_add_child (parent, id);

  bool put_status = put_user (f->eax, id);
  if( !put_status) syscall_t_exit (thread_current () -> name, -1);
}

static void 
syscall_wait( char * args, struct intr_frame *f ){
  bool put_status = put_user (f->eax, process_wait (args[1]));
  if( !put_status) syscall_t_exit (thread_current () -> name, -1);
}


static void 
syscall_create( char * args, struct intr_frame *f ){
  filesys_lock_acquire ();
  bool put_status = put_user (f->eax, filesys_create (args[1], args[2]));
  filesys_lock_release ()
  if( !put_status) syscall_t_exit (thread_current () -> name, -1);
}

static void 
syscall_remove( char * args, struct intr_frame *f ){
  filesys_lock_acquire ();
  bool put_status = put_user (f->eax, filesys_remove (args[1]));
  filesys_lock_release ()
  if( !put_status) syscall_t_exit (thread_current () -> name, -1);
}

static void 
syscall_open( char * args, struct intr_frame *f ){
  if( args[1] != 0 && args[1] != 1 ){
    
    filesys_lock_acquire ();
    struct file * file = filesys_open (args[2]);
    filesys_lock_release ()
    
    if(file == NULL) return -1;
    int fd = thread_add_file (file);

    bool put_status = put_user (f->eax, fd);
    if( !put_status) syscall_t_exit (thread_current () -> name, -1);
  }
}

static void 
syscall_filesize( char * args, struct intr_frame *f ){
  struct file_handle * fh = thread_get_file (args[1]);
  if( dh == NULL ) syscall_t_exit (thread_current () -> name, -1);

  bool put_status = put_user (f->eax, file_length (fh->file));
  if( !put_status ) syscall_t_exit (thread_current () -> name, -1);
}


static void 
syscall_read( char * args, struct intr_frame *f ){
  if( args[1] == 0){
    int i = 0;

    filesys_lock_acquire ();
    for( i; i < args[3]; i++){
      args[2][i] = input_getc();      
    }
    filesys_lock_release ()

    bool put_status = put_user (f -> eax, args[3]);
    if( !put_status) syscall_t_exit (thread_current () -> name, -1);
  } else if(args[1] == 1){
    //ERROR - we are trying to read from output :P
  } else {
    struct file * fh = thread_get_file (args[1]); 
    if( fh == NULL) syscall_t_exit (thread_current () -> name, -1);

    filesys_lock_acquire ();
    off_t written = file_read (fh->file, args[2], args[3]);
    filesys_lock_release ()

    bool put_status = put_user (f->eax, written);
    if( !put_status) syscall_t_exit (thread_current () -> name, -1);
  }
}

static void 
syscall_write( char * args, struct intr_frame *f ){
  if( args[1] == 0){
    //ERROR - we are trying to write to input :P
  } else if(args[1] == 1){
    uint8_t buffer = args[2];
    size_t size = args[3];
    
    int written = 0;
    filesys_lock_acquire ();
    
    if(size < 512) putbuf (buffer, size);
    else {
      while( size > 512 ){
        putbuf (buffer[written], 512);
        size-=512;
        written+=512;
      }
      putbuf (buffer[written], size);
      written+= size;
    }
    
    filesys_lock_release ()

    bool put_status = put_user (f -> eax, written);
  } else {
    struct file_handle * fh = thread_get_file (args[1]); 
    if( fh == NULL) syscall_t_exit (thread_current () -> name, -1);

    filesys_lock_acquire ();
    off_t written = file_write (fh->file, buffer, size);
    filesys_lock_release ()

    bool put_status = put_user (f -> eax, written);
    if( !put_status) syscall_t_exit (thread_current () -> name, -1);
  }
}

static void 
syscall_seek( char * args, struct intr_frame *f ){
  struct file_handle * fh = thread_get_file (args[1]);
  if( fh == NULL) syscall_t_exit (thread_current () -> name, -1);
  
  filesys_lock_acquire ();
  file_seek (fh->file, args[2]);
  filesys_lock_release ()
}

static void 
syscall_tell( char * args, struct intr_frame *f ){
  struct file_handle * fh = thread_get_file (args[1]);
  if( fh == NULL) syscall_t_exit (thread_current () -> name, -1);

  filesys_lock_acquire ();
  off_t position = file_tell (fh->file);
  bool put_status = put_user (f->eax, position);
  filesys_lock_release ()
  if( !put_status) syscall_t_exit (thread_current () -> name, -1);
}

static void 
syscall_close( char * args, struct intr_frame *f ){
  struct file_handle * fh = thread_get_file (args[1]);
  if( fh == NULL) syscall_t_exit (thread_current () -> name, -1);

  filesys_lock_acquire ();
  thread_remove_file (args[1]); //Remove file from files table
  file_close (fh -> file);      //Close file in the system
  filesys_lock_release ()
}
