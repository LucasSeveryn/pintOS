#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void filesys_lock_release (void);
void filesys_lock_acquire (void);
void syscall_t_exit (char *, int);

#endif /* userprog/syscall.h */
