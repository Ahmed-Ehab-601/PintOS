#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

/**
 * Returned by `wait(pid_t pid)` syscall if:
 * - `pid` does not refer to a direct child of the calling process.
 * - The process that calls wait has already called wait on `pid`.
 */

void syscall_init (void);
void* conv_virtual (void * esp);
int get_number_of_args(int systemCall);
void load_args(int *args, int numberOfArgs, int * esp);
void exit(int status);
void halt(void);

// /* Projects 2 and later. */
// void halt (void) NO_RETURN;
// void exit (int status) NO_RETURN;
// pid_t exec (const char *file);
// int wait (pid_t);
// bool create (const char *file, unsigned initial_size);
// bool remove (const char *file);
// int open (const char *file);
// int filesize (int fd);
// int read (int fd, void *buffer, unsigned length);
// int write (int fd, const void *buffer, unsigned length);
// void seek (int fd, unsigned position);
// unsigned tell (int fd);
// void close (int fd);

#endif /* userprog/syscall.h */
