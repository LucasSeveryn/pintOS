#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "filesys/filesys.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "vm/page.h"
#include "vm/frame.h"

#include "userprog/pagedir.h"
#include <kernel/stdio.h>

static void syscall_handler (struct intr_frame *);
static int * syscall_retrieve_args (struct intr_frame *);

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
static void syscall_mmap (int *, struct intr_frame *);
static void syscall_munmap (int *, struct intr_frame *);

static void (*syscall_functions[NOA]) (int* , struct intr_frame *); /* Array of syscall functions */
static struct lock filesys_lock;  /* File system lock */

static int syscall_noa[NOA];  /* Array with number of arguments for every syscall */

static bool DEBUG = false;

/* Reads a word at user virtual address UADDR.
UADDR must be below PHYS_BASE.
Returns the word value if successful, -1 if a segfault occurred. */
static int
get_word_user (const int *uaddr)
{
  int result;
  if ((void *) uaddr >= PHYS_BASE)
  {
    syscall_t_exit (thread_current () -> name, -1);
  }
  asm ("movl $1f, %0; movl %1, %0; 1:"
    : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Reads a byte at user virtual address UADDR.
UADDR must be below PHYS_BASE.
Returns the byte value if successful, terminates process on failure.
Due to compiler optimization not returning result causes the assembly
snippet to be removed. */
static int
validate_user (const uint8_t *uaddr)
{
  int result;
  if ((void *) uaddr >= PHYS_BASE)
  {
    syscall_t_exit (thread_current () -> name, -1);
  }
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
    : "=&a" (result) : "m" (*uaddr));
  return result;
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
  syscall_functions[SYS_MMAP] = &syscall_mmap;
  syscall_functions[SYS_MUNMAP] = &syscall_munmap;

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
  syscall_noa[SYS_MMAP] = 2;
  syscall_noa[SYS_MUNMAP] = 1;
}

static void
syscall_handler (struct intr_frame *f)
{
  struct thread *t = thread_current ();
  t->esp = f->esp;
  int syscall_number = get_word_user((int *)(f -> esp));
  if(syscall_number < SYS_HALT || syscall_number > SYS_MUNMAP){
    syscall_t_exit (t -> name, -1);
  }

  int *args = syscall_retrieve_args (f);
  syscall_functions[syscall_number] (args, f);
  free (args);
}

/* Function which wrapps everything that has to be done when calling exit */
void
syscall_t_exit (char * p_name, int status)
{
  thread_current () -> ret = status;
  printf ("%s: exit(%d)\n", p_name, status);
  thread_exit ();
}

/* Returns array of arguments retrieved from the frame */
static int *
syscall_retrieve_args (struct intr_frame *f)
{
  int *args = (int*) malloc(3);
  int syscall_number = get_word_user((int *)(f -> esp));
  int noa = syscall_noa[ syscall_number ];

  int i;
  for(i = 0; i <= noa; i++){
    args[i] = get_word_user((int *)(f -> esp) + i);
    if(args[i] == -1){
      syscall_t_exit (thread_current () -> name, -1);
    }
  }

  return args;
}

/* halt() - Stops the system end terminates */
static void
syscall_halt (int *args UNUSED, struct intr_frame *f UNUSED)
{
  shutdown_power_off ();
}

/* exit( int ) - Terminates the current program and returns its status */
static void
syscall_exit (int *args, struct intr_frame *f)
{
  f->eax = args[1];
  syscall_t_exit (thread_current () -> name, args[1]);
}

/* pid_t exex( const char * ) - Runs the executable whose name is given as the parameter */
static void
syscall_exec (int *args, struct intr_frame *f)
{
  validate_user((uint8_t *)args[1]);

  filesys_lock_acquire ();

  tid_t id = process_execute ((char *)args[1]);

  filesys_lock_release ();

  f->eax = id;
}

/* int wait( pid_t ) - waits for the child process with tid equals to the parameter
 * to terminate, and returns its status */
static void
syscall_wait (int *args, struct intr_frame *f )
{
  f->eax = process_wait (args[1]);
}

/* bool create( const char *, unsigned ) - create a new file of given size */
static void
syscall_create (int *args, struct intr_frame *f )
{
  validate_user ((uint8_t *) args[1]);

  filesys_lock_acquire ();
  f->eax = filesys_create ((char *) args[1], args[2]);
  filesys_lock_release ();
}

/* bool remove( const char * ) - Deletes a file */
static void
syscall_remove (int *args, struct intr_frame *f )
{
  validate_user ((uint8_t *) args[1]);

  filesys_lock_acquire ();
  f->eax = filesys_remove ((char*)args[1]);
  filesys_lock_release ();
}

/* bool open( const char * ) - Opens a file */
static void
syscall_open (int *args, struct intr_frame *f )
{
  validate_user ((uint8_t *) args[1]);

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

/* int filesize( int ) - Returns the size of a file */
static void
syscall_filesize (int *args, struct intr_frame *f )
{
  struct thread * t = thread_current();
  struct file_handle * fh = thread_get_file (&t->files, args[1]);
  if( fh == NULL ) syscall_t_exit (t->name, -1);

  f->eax = file_length (fh->file);
}

/* int read( int, void *, unsigned ) - Reads given number of bytes from the file into the buffer */
static void
syscall_read (int *args, struct intr_frame *f )
{
  struct thread * t = thread_current();
  if( args[1] == 0){
    int i = 0;
    uint8_t * buffer = (uint8_t *) args[2];
    validate_user (buffer);

    filesys_lock_acquire ();
    frame_pin (buffer, args[3]);
    for( ; i < args[3]; i++){
      buffer[i] = input_getc();
    }
    frame_unpin (buffer, args[3]);
    filesys_lock_release ();

    f -> eax = args[3];
  } else if(args[1] == 1){
    //ERROR - we are trying to read from output :P
  } else {
    uint8_t * buffer = (uint8_t *) args[2];
    validate_user (buffer);

    struct file_handle * fh = thread_get_file (&t->files, args[1]);
    if( fh == NULL ) syscall_t_exit (t->name, -1);

    void * br = malloc (args[3]);

    filesys_lock_acquire ();
    off_t written = file_read (fh->file, br, args[3]);
    filesys_lock_release ();

    frame_pin (buffer, args[3]);
    memcpy (buffer, br, args[3]);
    frame_unpin (buffer, args[3]);
    free(br);

    f->eax = written;
  }
}

/* int write( int, void *, unsigned ) - Writes given number of bytes to the file from the buffer */
static void
syscall_write (int *args, struct intr_frame *f)
{
  struct thread * t = thread_current();
  if( args[1] == 0){
    //ERROR - we are trying to write to input :P
  } else if(args[1] == 1){

    uint8_t * buffer =  (uint8_t *)args[2];
    validate_user (buffer);

    size_t size = args[3];

    int written = 0;
    filesys_lock_acquire ();
    frame_pin (buffer, args[3]);

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

    frame_unpin (buffer, args[3]);
    filesys_lock_release ();

    f -> eax = written;
  } else {
    uint8_t * buffer = (uint8_t *) args[2];
    validate_user (buffer);

    struct file_handle * fh = thread_get_file (&t->files, args[1]);
    if( fh == NULL ) syscall_t_exit (t->name, -1);

    void * br = malloc (args[3]);

    frame_pin (buffer, args[3]);
    memcpy (br, buffer, args[3]);
    frame_unpin (buffer, args[3]);

    filesys_lock_acquire ();
    off_t written = file_write (fh->file, br, args[3]);
    filesys_lock_release ();

    free (br);

    f -> eax = written;
  }
}

/* void seek( int, unsigned ) - Changes the next byte to be read or written from the file to the position */
static void
syscall_seek (int *args, struct intr_frame *f UNUSED)
{
  struct thread * t = thread_current();
  struct file_handle * fh = thread_get_file (&t->files, args[1]);
  if( fh == NULL) syscall_t_exit (t->name, -1);

  filesys_lock_acquire ();
  file_seek (fh->file, args[2]);
  filesys_lock_release ();
}

/* unsigned tell( int ) - Returns the position of the next byte to be read or written */
static void
syscall_tell (int *args, struct intr_frame *f){
  struct thread * t = thread_current();
  struct file_handle * fh = thread_get_file (&t->files, args[1]);
  if( fh == NULL) syscall_t_exit (t->name, -1);

  filesys_lock_acquire ();
  off_t position = file_tell (fh->file);
  f->eax = position;
  filesys_lock_release ();
}

/* void close( int ) - Closes a file with the given descriptor */
static void
syscall_close (int *args, struct intr_frame *f UNUSED)
{
  struct thread * t = thread_current();
  struct file_handle * fh = thread_get_file (&t->files, args[1]);
  if( fh == NULL) syscall_t_exit (t -> name, -1);
  filesys_lock_acquire ();
  file_close (fh -> file);      //Close file in the system
  thread_remove_file (fh); //Remove file from files table
  filesys_lock_release ();
}

/* void close( int ) - Closes a file with the given descriptor */
static void
syscall_mmap (int *args, struct intr_frame *f UNUSED)
{
  struct thread * t = thread_current ();

  if( args[1] == 0 || args[1] == 1){
    f->eax = -1;
    return;
  }
  struct file_handle * fh = thread_get_file (&t->files, args[1]);
  if( fh == NULL) syscall_t_exit (t -> name, -1);

  size_t fl = file_length (fh->file);
  if( fl == 0 || args[2] == 0 || args[2] % PGSIZE > 0){
    f->eax = -1;
    return;
  }

  // Book the memory
  int mmap_fd = thread_add_mmap_file (file_reopen (fh->file));
  struct file_handle * mmap_fh = thread_get_file (&t->mmap_files, mmap_fd);

  void * upage = (void*)args[2];
  mmap_fh->upage = upage;
  int pages = fl / PGSIZE;
  if( fl % PGSIZE > 0 ){
    pages++;
  }

  int i;
  for( i = 0; i < pages; i++ ){
    size_t zero_after = ( i == pages - 1) ? fl % PGSIZE : PGSIZE;
    off_t offset = i * PGSIZE;
    struct suppl_page *new_page = new_file_page(mmap_fh->file, offset, zero_after, true, FILE);

    sema_down(t->pagedir_mod);
    void * overlapControl = pagedir_get_page(t->pagedir, upage + offset);
    sema_up(t->pagedir_mod);

    if( overlapControl != 0 ){
      free (new_page);
      f->eax = -1;
      return;
    }
    sema_down(t->pagedir_mod);
    pagedir_set_page_suppl (t->pagedir, upage + offset, new_page);
    sema_up(t->pagedir_mod);
  }

  f->eax = mmap_fd;
}

/* void close( int ) - Closes a file with the given descriptor */
static void
syscall_munmap (int *args, struct intr_frame *f UNUSED)
{
  struct thread * t = thread_current ();
  struct file_handle * fh = thread_get_file (&t->mmap_files, args[1]);
  void * upage = fh->upage;
  size_t fl = file_length (fh->file);
  int pages = fl / PGSIZE;
  if( fl % PGSIZE > 0 ){
    pages++;
  }

  int i;
  for( i = 0; i < pages; i++ ){
    void * uaddr = upage + i*PGSIZE;
    sema_down(t->pagedir_mod);
    bool dirty = pagedir_is_dirty (t->pagedir, uaddr);
    uint32_t kpage = (uint32_t) pagedir_get_page(t->pagedir, uaddr);
    sema_up(t->pagedir_mod);
    if((kpage & PTE_P) != 0 && dirty) {
      int zero_after = ( i == pages - 1) ? fl%PGSIZE : PGSIZE;
      file_seek (fh->file, i*PGSIZE);

      lock_frames();
      frame_pin (uaddr, PGSIZE);
      unlock_frames();

      filesys_lock_acquire();
      file_write (fh->file, uaddr, zero_after);
      filesys_lock_release();

      lock_frames();
      frame_unpin (uaddr, PGSIZE);
      unlock_frames();
    }
    sema_down(t->pagedir_mod);
    pagedir_clear_page (t->pagedir, uaddr);
    sema_up(t->pagedir_mod);
  }

  list_remove (&fh->elem);
  file_close (fh->file);
  free (fh);
}
