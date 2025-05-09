// #ifndef USERPROG_FILESYSCALL_H
// #define USERPROG_FILESYSCALL_H

// #include "threads/synch.h"
// struct lock filesys_lock, std_input_lock, std_output_lock;

// #define INVALID_FD -1
// #define EXIT_FAILURE -1
// #define INVALID_FILE -1

// struct file_descriptor {
// 	struct file *file;	  		/* File pointer. */
// 	int fd;						/* File descriptor. */
// 	struct list_elem elem; 		/* List element for file descriptor list. */
// };

// bool create (const char *file_name, unsigned initial_size);
// bool remove (const char *file_name);
// int open (const char *file_name);
// int filesize (int fd_num);
// int read (int fd_num, void *buffer, unsigned size);
// int write (int fd_num, const void *buffer, unsigned size);
// void seek (int fd_num, unsigned position);
// unsigned tell (int fd_num);
// void close (int fd_num);
// void close_all_files (struct thread *thread);

// #endif