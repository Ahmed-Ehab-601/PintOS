#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "kernel/list.h"
struct lock filesys_lock, std_input_lock, std_output_lock;

#define INVALID_FD -1
#define EXIT_FAILURE -1
#define INVALID_FILE -1
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

struct file_descriptor {
	struct file *file;	  		/* File pointer. */
	int fd;						/* File descriptor. */
	struct list_elem elem; 		/* List element for file descriptor list. */
};

/**
 * Returned by `wait(pid_t pid)` syscall if:
 * - `pid` does not refer to a direct child of the calling process.
 * - The process that calls wait has already called wait on `pid`.
 */

void syscall_init (void);
void* conv_virtual (void * esp);
void verify_str (const void* str);
static void verify_buffer (void* buffer, unsigned size);
int get_number_of_args(int systemCall);
void load_args(int *args, int numberOfArgs, int * esp);
void exit(int status);
void handle_halt(void);

/* Projects 2 and later. */
// void halt (void);
// void exit (int status);
// int exec (const char *file);
// int wait (tid_t child);
// bool create (const char *file, unsigned initial_size);
// bool remove (const char *file);
// int open (const char *file);
// int filesize (int fd);
// int read (int fd, void *buffer, unsigned length);
// int write (int fd, const void *buffer, unsigned length);
// void seek (int fd, unsigned position);
// unsigned tell (int fd);
// void close (int fd);

bool create (const char *file_name, unsigned initial_size);
bool remove (const char *file_name);
int open (const char *file_name);
int filesize (int fd_num);
int read (int fd_num, void *buffer, unsigned size);
int write (int fd_num, const void *buffer, unsigned size);
void seek (int fd_num, unsigned position);
unsigned tell (int fd_num);
void close (int fd_num);

#endif /* userprog/syscall.h */
