#include "userprog/syscall.h"

#include <stdio.h>
#include <syscall-nr.h>

#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

static void syscall_handler(struct intr_frame *);

bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd_num);
int read(int fd_num, void *buffer, unsigned size);
int write(int fd_num, const void *buffer, unsigned size);
void seek(int fd_num, unsigned position);
unsigned tell(int fd_num);
void close(int fd_num);

tid_t exec(const char *cmd_line);
int wait(tid_t child);

struct file_descriptor *get_file_descriptor(int fd_num, struct thread *t);
int add_file(struct file *f, struct thread *t);

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

static void syscall_handler(struct intr_frame *f) {
	int args[3];
	void * esp = conv_virtual(f->esp);
	int systemCall = *(int *)esp;
	if (systemCall < SYS_HALT || systemCall > SYS_INUMBER){
		exit(-1);
	}
	int numberOfArgs = get_number_of_args(systemCall);
	load_args(args, numberOfArgs, (int *)f->esp);

	switch (systemCall) {
	case SYS_HALT:
		handle_halt();
		break;
	case SYS_EXIT:
		exit(args[0]);
		break;
	case SYS_WAIT:
		f->eax = wait(args[0]);
		break;
	case SYS_EXEC:
		verify_str(args[0]);
		f->eax = exec(args[0]);
		break;
	case SYS_CREATE:	// what is the "eax" ?
		verify_str(args[0]);
		f->eax = create(args[0], args[1]);
		break;
	case SYS_REMOVE:
		verify_str(args[0]);
		f->eax = remove(args[0]);
		break;
	case SYS_OPEN:
		verify_str(args[0]);
		f->eax = open(args[0]);
		break;
	case SYS_FILESIZE:
		f->eax = filesize(args[0]);
		break;
	case SYS_READ:
		verify_buffer(args[1], args[2]);
		f->eax = read(args[0], args[1], args[2]);
		break;
	case SYS_WRITE:
		verify_buffer(args[1], args[2]);
		f->eax = write(args[0], args[1], args[2]);
		break;
	case SYS_SEEK:
		seek(args[0], args[1]);
		break;
	case SYS_TELL:
		f->eax = tell(args[0]);
		break;
	case SYS_CLOSE:
		close(args[0]);
		break;
	default:
		break;
	}
}

void load_args(int *args, int numberOfArgs, int *esp) {
	for (int i = 0; i < numberOfArgs; i++) {
		args[i] = *(int *)conv_virtual(esp + (i + 1));
	}
}

 void verify_str (const void* str){
	char* toCheck = *(char*) conv_virtual (str);
	for ( toCheck; toCheck != 0; toCheck = *(char*) conv_virtual(++str)) ;
}

static void verify_buffer (void* buffer, unsigned size){
	int i = 0;
	char* temp = (char*) buffer;
	while( i < size){
	conv_virtual((const void*) temp++);
	i++;
	}
}

int get_number_of_args(int systemCall) {
	switch (systemCall) {
	case SYS_HALT:
		return 0;
	case SYS_EXIT:
		return 1;
	case SYS_WAIT:
		return 1;
	case SYS_EXEC:
		return 1;
	case SYS_CREATE:
		return 2;
	case SYS_REMOVE:
		return 1;
	case SYS_OPEN:
		return 1;
	case SYS_FILESIZE:
		return 1;
	case SYS_READ:
		return 3;
	case SYS_WRITE:
		return 3;
	case SYS_SEEK:
		return 2;
	case SYS_TELL:
		return 1;
	case SYS_CLOSE:
		return 1;
	default:
		return 0;
		break;
	}
}

void *conv_virtual(void *esp) {
	if (!is_user_vaddr(esp)) {
		exit(-1);
	}
	void *ptr = pagedir_get_page(thread_current()->pagedir, esp);
	if (ptr == NULL) {
		exit(-1);
	}

	//return esp;
	return ptr;
}

void exit(int status) {
	// if it's child prosses handle waited parent here !!
	struct thread *cur = thread_current();
	printf("%s: exit(%d)\n", cur->name, status);
	release_all_locks();
	// close all files
	cur->parent->child_exit_status = status;
	sema_up(&cur->parent->is_running);
	/**
	 * Loop for all parent children and wake up children
	 * that are waiting (blocked) for parent
	 * 
	 */
	thread_exit();
}

void handle_halt(void) { 
	shutdown_power_off();
}

bool create(const char *file, unsigned initial_size) {
	if (file == NULL) {
		exit(-1);
	}
	return filesys_create(file, initial_size);
}

bool remove(const char *file) {
	if (file == NULL) {
		exit(-1);
	}
	return filesys_remove(file);
}

int open(const char *file) {
	if (file == NULL) {
		return -1;
	}
	struct file *f = filesys_open(file);
	if (f == NULL) {
		return -1;
		// exit(-1);
		// create(file, 100);
	}

	return add_file(f, thread_current());
}

int filesize(int fd_num) {
	struct file_descriptor *fd = get_file_descriptor(fd_num, thread_current());
	if (fd == NULL) {
		exit(-1);
	}
	return file_length(fd->file);
}

int read(int fd_num, void *buffer, unsigned size) {
	if (buffer == NULL || !is_user_vaddr(buffer)) {
		exit(-1);
	}

	if (fd_num == STDIN_FILENO) {
		unsigned i;
		uint8_t *buf = buffer;
		for (i = 0; i < size; i++) {
			buf[i] = input_getc();
		}
		return size;
	}

	struct file_descriptor *fd = get_file_descriptor(fd_num, thread_current());
	if (fd == NULL) {
		exit(-1);
	}

	lock_acquire(&fd->file->file_lock);
	int bytes_read = file_read(fd->file, buffer, size);
	lock_release(&fd->file->file_lock);

	return bytes_read;
}

int write(int fd_num, const void *buffer, unsigned size) {
	if (buffer == NULL || !is_user_vaddr(buffer)) {
		return -1;
	}

	if (fd_num == STDOUT_FILENO) {
		putbuf(buffer, size);
		return size;
	}

	struct file_descriptor *fd = get_file_descriptor(fd_num, thread_current());
	if (fd == NULL) {
		return -1;
	}

	lock_acquire(&fd->file->file_lock);
	int bytes_written = file_write(fd->file, buffer, size);
	lock_release(&fd->file->file_lock);

	return bytes_written;
}

void seek(int fd_num, unsigned position) {
	struct file_descriptor *fd = get_file_descriptor(fd_num, thread_current());
	if (fd == NULL) {
		exit(-1);
	}
	lock_acquire(&fd->file->file_lock);
	file_seek(fd->file, position);
	lock_release(&fd->file->file_lock);
}

unsigned tell(int fd_num) {
	struct file_descriptor *fd = get_file_descriptor(fd_num, thread_current());
	if (fd == NULL) {
		exit(-1);
	}

	lock_acquire(&fd->file->file_lock);
	unsigned position = file_tell(fd->file);
	lock_release(&fd->file->file_lock);

	return position;
}

void close(int fd_num) {
	struct file_descriptor *fd = get_file_descriptor(fd_num, thread_current());

	if (fd == NULL) {
		exit(-1);
	}

	lock_acquire(&fd->file->file_lock);
	file_close(fd->file);
	list_remove(&fd->elem);
	free(fd);
}

struct file_descriptor *get_file_descriptor(int fd_num, struct thread *t) {
	if (fd_num < 0)
		return NULL;

	for (struct list_elem *e = list_begin(&t->file_descriptors); e != list_end(&t->file_descriptors);
		  e = list_next(e)) {
		struct file_descriptor *fd = list_entry(e, struct file_descriptor, elem);
		if (fd->fd == fd_num) {
			return fd;
		}
	}
	return NULL;
}

int add_file(struct file *f, struct thread *t) {
	if (f == NULL) {
		return -1;
	}

	struct file_descriptor *fd = malloc(sizeof(struct file_descriptor));
	if (fd == NULL) {
		return -1;
	}

	fd->file = f;
	fd->fd = t->next_fd++;
	list_push_back(&t->file_descriptors, &fd->elem);

	return fd->fd;
}

tid_t exec(const char *cmd_line) {
	return process_execute(cmd_line);
}

int wait(tid_t child) { return process_wait(child); }