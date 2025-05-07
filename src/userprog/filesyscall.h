#ifndef USERPROG_FILESYSCALL_H
#define USERPROG_FILESYSCALL_H

#include "threads/synch.h"
struct lock filesys_lock, std_input_lock, std_output_lock;

#define INVALID_FD -1
#define EXIT_FAILURE -1
#define INVALID_FILE -1

struct file_descriptor {
	struct file *file;	  		/* File pointer. */
	int fd;						/* File descriptor. */
	struct list_elem elem; 		/* List element for file descriptor list. */
};

bool file_create (const char *file_name, unsigned initial_size);
bool file_remove (const char *file_name);
int file_open (const char *file_name);
int file_size_fd (int fd_num);
int file_read_fd (int fd_num, void *buffer, unsigned size);
int file_write_fd (int fd_num, const void *buffer, unsigned size);
void file_seek_fd (int fd_num, unsigned position);
unsigned file_tell_fd (int fd_num);
void file_close_fd (int fd_num);
void file_close_all(struct thread *thread);

#endif