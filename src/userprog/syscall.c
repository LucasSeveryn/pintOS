#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "filesys/filesys.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include <kernel/stdio.h>

void (*syscall_functions[NOA]) (int* , struct intr_frame *);

#define DEBUG false

static void syscall_handler (struct intr_frame *);
static int * syscall_retrieve_args (struct intr_frame *);

static void filesys_lock_release (void);
static void filesys_lock_acquire (void);

static void syscall_t_exit (char *, int);

static void syscall_halt (int *, struct intr_frame *);
static void syscall_exit (int *, struct intr_frame *);
static void syscall_exec (int *, struct intr_frame *);
static void syscall_wait (int *, struct intr_frame *);
static void syscall_create (int *, struct intr_frame *);
static void syscall_remove (int *, struct intr_frame *);
static void syscall_open (int *, struct intr_frame *);
static void syscall_filesize (int *, struct intr_frame *);
static void syscall_read (int *, struct intr_frame *);
static void syscall_write (int *, struct intr_frame *);
static void syscall_seek (int *, struct intr_frame *);
static void syscall_tell (int *, struct intr_frame *);
static void syscall_close (int *, struct intr_frame *);

static struct lock filesys_lock;
static int syscall_noa[NOA];

/* Reads a byte at user virtual address UADDR.
UADDR must be below PHYS_BASE.
Returns the byte value if successful, -1 if a segfault occurred. */
static int
get_user (const uint8_t *uaddr)
{
	int result;
	if ((void *) uaddr >= PHYS_BASE)
	{
		if(DEBUG) printf ("Trying to access memory address: %p, which is kernel memory address\n", uaddr);
    syscall_t_exit (thread_current () -> name, -1);
	}
	asm ("movl $1f, %0; movzbl %1, %0; 1:"
		: "=&a" (result) : "m" (*uaddr));
	return result;
}

/* Reads a byte at user virtual address UADDR.
UADDR must be below PHYS_BASE.
Returns the byte value if successful, -1 if a segfault occurred. */
static int
get_user_word (const int *uaddr)
{
  int result;
  if ((void *) uaddr >= PHYS_BASE)
  {
    if(DEBUG) printf ("Trying to access memory address: %p, which is kernel memory address\n", uaddr);
    syscall_t_exit (thread_current () -> name, -1);
  }
  asm ("movl $1f, %0; movl %1, %0; 1:"
    : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST. UDST must be below PHYS_BASE.
Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
	int error_code;
	if ((void *) udst >= PHYS_BASE)
	{
		if(DEBUG) printf ("Trying to write to memory address: %p, value %u.\n",
				 udst, byte);
		return false;
	}
	asm ("movl $1f, %0; movb %b2, %1; 1:"
		: "=&a" (error_code), "=m" (*udst) : "q" (byte));
	return error_code != -1;
}

static int
get_safe(int * addr){
  int res = 0;
  int tmp;

  int i;
  for( i = 3; i >= 0; i-- ){
    tmp = get_user((uint8_t*)(addr) + i);
    if(DEBUG) printf("Byte %d - value %d | ", i, tmp);
    if(tmp == -1){
      syscall_t_exit (thread_current () -> name, -1);
    }
    res |= tmp;
    if(i != 0) res <<= 8;
  }
  if(DEBUG) printf(" Result: %d.\n", res);
  return res;
}

void
filesys_lock_release ()
{
  lock_release (&filesys_lock);
};

void
filesys_lock_acquire ()
{
  lock_acquire (&filesys_lock);
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
  syscall_functions[SYS_TELL] = &syscall_tell;
  syscall_functions[SYS_CLOSE] = &syscall_close;

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
  if( f->eax == -1 ){
    syscall_t_exit (thread_current () -> name, -1);
  }

  int syscall_number = get_word_user((int *)(f -> esp));
  if(syscall_number < SYS_HALT || syscall_number > SYS_CLOSE){
    syscall_t_exit (thread_current () -> name, -1);
  }

  int * args = syscall_retrieve_args (f);
  syscall_functions[syscall_number] ( args, f );
  free (args);
}

static void
syscall_t_exit (char * p_name, int status)
{
  thread_current () -> ret = status;
  printf ("%s: exit(%d)\n", p_name, status);
  thread_exit ();
}

static int *
syscall_retrieve_args (struct intr_frame *f)
{
  int * args = (int*) malloc(3);
  int syscall_number = get_word_user((int*)(f -> esp));
  int noa = syscall_noa[ syscall_number ];

  int i;
  for(i = 0; i <= noa; i++){
    args[i] = get_word_user((int*)(f -> esp) + i);
    if(args[i] == -1){
      syscall_t_exit (thread_current () -> name, -1);
    }
  }

  return args;
}

static void
syscall_halt (int * args UNUSED, struct intr_frame *f UNUSED)
{
  shutdown_power_off ();
}

static void
syscall_exit (int * args, struct intr_frame *f)
{
  if( f->eax == -1 ){
    args[1] = -1;
  }

  f->eax = args[1];
  syscall_t_exit (thread_current () -> name, args[1]);
}

static void
syscall_exec (int * args, struct intr_frame *f)
{

  struct thread * parent = thread_current();

  char validate = get_user((char*)args[1]);
  if(validate == -1) syscall_t_exit (thread_current () -> name, -1);

  filesys_lock_acquire ();

  tid_t id = process_execute ((char*)args[1]);

  filesys_lock_release ();

  f->eax = id;
}

static void
syscall_wait (int * args, struct intr_frame *f )
{
  f->eax = process_wait (args[1]);
}


static void
syscall_create (int * args, struct intr_frame *f )
{
  char validate = get_user ((uint8_t *) args[1]);
  if(validate == -1) syscall_t_exit (thread_current () -> name, -1);

  filesys_lock_acquire ();
  f->eax = filesys_create ((char*)args[1], args[2]);
  filesys_lock_release ();
}

static void
syscall_remove (int * args, struct intr_frame *f )
{
  char validate = get_user ((uint8_t *) args[1]);
  if(validate == -1) syscall_t_exit (thread_current () -> name, -1);

  filesys_lock_acquire ();
  f->eax = filesys_remove ((char*)args[1]);
  filesys_lock_release ();
}

static void
syscall_open (int * args, struct intr_frame *f )
{
  char validate = get_user ((uint8_t *) args[1]);
  if(validate == -1) syscall_t_exit (thread_current () -> name, -1);

  filesys_lock_acquire ();
  struct file * file = filesys_open ((char *)args[1]);
  filesys_lock_release ();

  int fd;
  if(file == NULL) {
    fd = -1;
  } else {
    fd = thread_add_file (file);
  }

  f->eax = fd;
}

static void
syscall_filesize (int * args, struct intr_frame *f )
{
  struct file_handle * fh = thread_get_file (args[1]);
  if( fh == NULL ) syscall_t_exit (thread_current () -> name, -1);

  f->eax = file_length (fh->file);
}


static void
syscall_read (int * args, struct intr_frame *f )
{
  if( args[1] == 0){
    int i = 0;
    uint8_t * buffer = (uint8_t *) args[2];
    char validate = get_user (buffer);
    if(validate == -1) syscall_t_exit (thread_current () -> name, -1);

    filesys_lock_acquire ();
    for( ; i < args[3]; i++){
      buffer[i] = input_getc();
    }
    filesys_lock_release ();

    f -> eax = args[3];
  } else if(args[1] == 1){
    //ERROR - we are trying to read from output :P
  } else {
    uint8_t * buffer = (uint8_t *) args[2];
    int validate = get_user (buffer);
    if(validate == -1) {
      f -> eax = -1;
      return;
    }

    struct file_handle * fh = thread_get_file (args[1]);
    if( fh == NULL) {
      f -> eax = -1;
      return;
    }

    filesys_lock_acquire ();
    off_t written = file_read (fh->file, buffer, args[3]);
    filesys_lock_release ();

    f->eax = written;
  }
}

static void
syscall_write (int * args, struct intr_frame *f)
{
  if( args[1] == 0){
    //ERROR - we are trying to write to input :P
  } else if(args[1] == 1){

    uint8_t * buffer =  (uint8_t *)args[2];
    char validate = get_user (buffer);
    if(validate == -1) syscall_t_exit (thread_current () -> name, -1);

    size_t size = args[3];

    int written = 0;
    filesys_lock_acquire ();

    if(size < 512) putbuf ((char*)buffer, size);
    else {
      while( size > 512 ){
        putbuf ((char *)(buffer + written), 512);
        size-=512;
        written+=512;
      }
      putbuf ((char*)(buffer + written), size);
      written+= size;
    }

    filesys_lock_release ();

    f -> eax = written;
  } else {
    uint8_t * buffer = (uint8_t *) args[2];
    int validate = get_user (buffer);
    if(validate == -1) {
      f -> eax = 0;
      return;
    }

    struct file_handle * fh = thread_get_file (args[1]);
    if( fh == NULL) {
      f->eax = 0;
      return;
    }

    filesys_lock_acquire ();
    off_t written = file_write (fh->file, buffer, args[3]);
    filesys_lock_release ();

    f -> eax = written;
  }
}

static void
syscall_seek (int * args, struct intr_frame *f UNUSED)
{
  struct file_handle * fh = thread_get_file (args[1]);
  if( fh == NULL) syscall_t_exit (thread_current () -> name, -1);

  filesys_lock_acquire ();
  file_seek (fh->file, args[2]);
  filesys_lock_release ();
}

static void
syscall_tell (int * args, struct intr_frame *f){
  struct file_handle * fh = thread_get_file (args[1]);
  if( fh == NULL) syscall_t_exit (thread_current () -> name, -1);

  filesys_lock_acquire ();
  off_t position = file_tell (fh->file);
  f->eax = position;
  filesys_lock_release ();
}

static void
syscall_close (int * args, struct intr_frame *f UNUSED)
{
  struct thread * t = thread_current();

  struct file_handle * fh = thread_get_file (args[1]);
  if( fh == NULL) syscall_t_exit (t -> name, -1);
  filesys_lock_acquire ();
  file_close (fh -> file);      //Close file in the system
  thread_remove_file (fh); //Remove file from files table
  filesys_lock_release ();
}
