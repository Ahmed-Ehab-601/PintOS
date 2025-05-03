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

static void syscall_handler(struct intr_frame *);

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

static void syscall_handler(struct intr_frame *f) {
	int args[3];
	void *esp = conv_virtual(f->esp);
	int systemCall = *(int *)esp;
	int numberOfArgs = get_number_of_args(systemCall);
	load_args(args, numberOfArgs, (int *)f->esp);
	switch (systemCall) {
	case SYS_HALT:
		halt();
		break;
	case SYS_EXIT:
		exit(args[0]);
		break;
	case SYS_WAIT:
		/* code */
		break;
	case SYS_EXEC:
		/* code */
		break;
	case SYS_CREATE:
		/* code */
		break;
	case SYS_REMOVE:
		/* code */
		break;
	case SYS_OPEN:
		/* code */
		break;
	case SYS_FILESIZE:
		/* code */
		break;
	case SYS_READ:
		/* code */
		break;
	case SYS_WRITE:
		/* code */
		break;
	case SYS_SEEK:
		/* code */
		break;
	case SYS_TELL:
		/* code */
		break;
	case SYS_CLOSE:
		/* code */
		break;
	default:
		break;
	}
}

void load_args(int *args, int numberOfArgs, int *esp) {
	for (int i = 0; i < numberOfArgs; i++) {
		args[i] = *(int *)conv_virtual(esp + i + 1);
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
	if (esp < (int *)0x08048000 || esp >= (int *)PHYS_BASE) {
		exit(-1);
	}
	void *ptr = pagedir_get_page(thread_current()->pagedir, esp);
	if (ptr == NULL) {
		exit(-1);
	}
	return esp;
	// return ptr; !!!!!
}

void exit(int status) {
	// if it's child prosses handle waited parent here !!
	struct thread *cur = thread_current();
	printf("%s: exit(%d)\n", cur->name, status);
	release_all_locks();
	// close all files
	thread_exit();
}
void halt(void) { shutdown_power_off(); }