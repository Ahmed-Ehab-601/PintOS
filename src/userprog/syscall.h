#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "kernel/list.h"
struct lock filesys_lock, std_input_lock, std_output_lock, exec_lock;

#define INVALID_FD -1
#define EXIT_FAILURE -1
#define INVALID_FILE -1
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

struct file_descriptor
{
	struct file *file;	   /* File pointer. */
	int fd;				   /* File descriptor. */
	struct list_elem elem; /* List element for file descriptor list. */
};

void syscall_init(void);
void *check_address(void *esp);
void check_string( void *str);
static void check_buffer(void *buffer, unsigned size);
int get_number_of_args(int systemCall);
void load_args(int *args, int numberOfArgs, int *esp);
void exit(int status);
void handle_halt(void);

bool create(const char *file_name, unsigned initial_size);
bool remove(const char *file_name);
int open(const char *file_name);
int filesize(int fd_num);
int read(int fd_num, void *buffer, unsigned size);
int write(int fd_num, const void *buffer, unsigned size);
void seek(int fd_num, unsigned position);
unsigned tell(int fd_num);
void close(int fd_num);

#endif /* userprog/syscall.h */
