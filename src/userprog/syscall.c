#include "userprog/syscall.h"

#include <stdio.h>
#include <syscall-nr.h>

#include "devices/shutdown.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/filesyscall.h"

static void syscall_handler(struct intr_frame *);
tid_t exec(const char *cmd_line);
int wait(tid_t child);
void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

static void syscall_handler(struct intr_frame *f) {
	int args[3];
	void *esp = conv_virtual(f->esp);
	int systemCall = *(int *)esp;
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
		f->eax = exec(args[0]);
		break;
	case SYS_CREATE:	// what is the "eax" ?
		conv_virtual(args[0]);
		f->eax = file_create(args[0], args[1]);
		break;
	case SYS_REMOVE:
		conv_virtual(args[0]);
		f->eax = file_remove(args[0]);
		break;
	case SYS_OPEN:
		printf("%s", args[0]);
		conv_virtual(args[0]);
		f->eax = file_open(args[0]);
		break;
	case SYS_FILESIZE:
		f->eax = file_size_fd(args[0]);
		break;
	case SYS_READ:
		printf("-------- %ud -------------\n", args[2]);
		f->eax = file_read(args[0], args[1], args[2]);
		break;
	case SYS_WRITE:
		f->eax = file_write(args[0], args[1], args[2]);
		break;
	case SYS_SEEK:
		file_seek(args[0], args[1]);
		break;
	case SYS_TELL:
		f->eax = file_tell_fd(args[0]);
		break;
	case SYS_CLOSE:
		file_close(args[0]);
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
	if (esp < (void *)0x08048000 || esp >= (void *)PHYS_BASE) {
		printf("in if in conv");
		exit(-1);
	}
	void *ptr = pagedir_get_page(thread_current()->pagedir, esp);
	if (ptr == NULL) {
		exit(-1);
	}

	return esp;
	// return ptr;
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
	file_close_all(cur);
	thread_exit();
}

void handle_halt(void) { 
	shutdown_power_off();
}

tid_t exec(const char *cmd_line) {
	return process_execute(cmd_line);
}

int wait(tid_t child) { return process_wait(child); }