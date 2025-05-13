#include "userprog/syscall.h"

#include <stdio.h>
#include <syscall-nr.h>

#include "threads/malloc.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "userprog/syscall.h"

int file_add_to_thread(struct file *file, struct thread *thread);
struct file_descriptor *file_get_fd(int fd_num, struct thread *thread);
void close(int fd_num);

static void syscall_handler(struct intr_frame *);
tid_t exec(const char *cmd_line);
int wait(tid_t e);

void syscall_init(void)
{
	intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");

	lock_init(&filesys_lock);
	lock_init(&std_input_lock);
	lock_init(&std_output_lock);
	lock_init(&exec_lock);
}
static void syscall_handler(struct intr_frame *f)
{
	int args[3];
	void *esp = check_address(f->esp);
	int systemCall = *(int *)esp;
	if (systemCall < SYS_HALT || systemCall > SYS_INUMBER)
	{
		exit(-1);
	}
	int numberOfArgs = get_number_of_args(systemCall);
	load_args(args, numberOfArgs, (int *)f->esp);

	switch (systemCall)
	{
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
		check_string(args[0]);
		lock_acquire(&exec_lock);
		f->eax = exec(args[0]);
		lock_release(&exec_lock);
		break;
	case SYS_CREATE:
		check_string(args[0]);
		f->eax = create(args[0], args[1]);
		break;
	case SYS_REMOVE:
		check_string(args[0]);
		f->eax = remove(args[0]);
		break;
	case SYS_OPEN:
		check_string(args[0]);
		f->eax = open(args[0]);
		break;
	case SYS_FILESIZE:
		f->eax = filesize(args[0]);
		break;
	case SYS_READ:
		check_buffer(args[1], args[2]);
		lock_acquire(&filesys_lock);
		f->eax = read(args[0], args[1], args[2]);
		lock_release(&filesys_lock);
		break;
	case SYS_WRITE:
		check_buffer(args[1], args[2]);
		lock_acquire(&filesys_lock);
		f->eax = write(args[0], args[1], args[2]);
		lock_release(&filesys_lock);
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

void load_args(int *args, int numberOfArgs, int *esp)
{
	for (int i = 0; i < numberOfArgs; i++)
	{
		args[i] = *(int *)check_address(esp + (i + 1));
	}
}

void check_string(void *str)
{
	char *toCheck = *(char *) check_address(str);
	for (toCheck; toCheck != 0; toCheck = *(char *)check_address(++str));
}

static void check_buffer(void *buffer, unsigned size)
{
	int i = 0;
	char *temp = (char *)buffer;
	while (i < size)
	{
		check_address((void *)temp++);
		i++;
	}
}

int get_number_of_args(int systemCall)
{
	switch (systemCall)
	{
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

void *check_address(void *esp)
{
	if (esp == NULL)
	{
		exit(-1);
	}
	if (esp < (void *)0x08048000 || esp >= (void *)PHYS_BASE)
	{
		exit(-1);
	}
	return esp;
}

void exit(int status)
{
	struct thread *cur = thread_current();
	printf("%s: exit(%d)\n", cur->name, status);
	release_all_locks();
	// close all files
	struct list_elem *e = list_begin(&cur->file_descriptors);
	while (e != list_end(&cur->file_descriptors))
	{
		struct file_descriptor *fd = list_entry(e, struct file_descriptor, elem);
		e = list_next(e);
		close(fd->fd);
	}

	sema_up(&cur->parent->is_running);
	cur->parent->child_exit_status = status;
	thread_exit();
}

void handle_halt(void)
{
	shutdown_power_off();
}

/**
 * Creates a new file with the given name and initial size.
 * Returns true on success, false otherwise.
 * Does not open the file.
 */
bool create(const char *file_name, unsigned initial_size)
{
	return file_name != NULL && filesys_create(file_name, initial_size);
}

/**
 * Deletes the file called file. Returns true if successful, false otherwise.
 * A file may be removed regardless of whether it is open or closed.
 */
bool remove(const char *file_name)
{
	return file_name != NULL && filesys_remove(file_name);
}

/**
 * Opens the file named `file_name` and returns a nonnegative file descriptor (fd),
 * or -1 if the file could not be opened.
 *
 * File descriptors 0 and 1 are reserved for the console:
 *   - fd 0 (STDIN_FILENO): standard input
 *   - fd 1 (STDOUT_FILENO): standard output
 * This function will never return these reserved descriptors.
 *
 * Each process maintains its own set of file descriptors, which are not
 * inherited by child processes.
 *
 * When a file is opened multiple times, whether by the same process or by
 * different processes, each call returns a unique file descriptor. These
 * descriptors operate independently, maintaining separate file positions and
 * requiring separate close operations.
 */
int open(const char *file_name)
{
	if (file_name == NULL)
		return INVALID_FILE;

	struct file *file = filesys_open(file_name);

	if (file == NULL)
		return INVALID_FILE;

	int fd = file_add_to_thread(file, thread_current());
	return fd;
}

/**
 * Returns the size, in bytes, of the file associated with the given file descriptor `fd_num`.
 * If the file descriptor is invalid or the file cannot be found, the function exits with a failure status.
 *
 * This function retrieves the file descriptor structure from the current thread using `file_get_fd` and then
 * retrieves the file length via `file_length`.
 * If the file descriptor is not valid or the file cannot be found, the program terminates with `EXIT_FAILURE`.
 */
int filesize(int fd_num)
{
	struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
	if (fd == NULL)
		return -1;

	int size = file_length(fd->file);

	return size;
}

/**
 * Reads `size` bytes from the file identified by `fd` into `buffer`.
 *
 * Returns the number of bytes actually read:
 * - 0 indicates end of file,
 * - -1 indicates an error occurred (other than end of file).
 *
 * If `fd` is 0 (standard input), input is read from the keyboard using `input_getc()`.
 */
int read(int fd_num, void *buffer, unsigned size)
{
	if (buffer == NULL || !is_user_vaddr(buffer))
		return -1;

	if (fd_num == STDIN_FILENO)
	{
		uint8_t *buf = buffer;
		for (unsigned i = 0; i < size; i++)
		{
			buf[i] = input_getc();
		}
		return size;
	}

	if (fd_num == STDOUT_FILENO)
		return -1;

	struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
	if (fd == NULL)
		return -1;

	int bytes_read = file_read(fd->file, buffer, size);

	return bytes_read;
}

/**
 * Writes `size` bytes from `buffer` to the file represented by `fd`.
 *
 * Returns the number of bytes actually written, which may be less than `size`
 * if some bytes could not be written. If the write attempts to go past the
 * end-of-file, only the bytes up to the end are written. File extension is not
 * supported by the basic file system, so such writes will not grow the file.
 * If no bytes can be written, the function returns 0.
 *
 * Special case:
 * - Writing to `fd == 1` (standard output) sends the output to the console.
 *   In this case, the entire buffer should be written in a single call to
 *   `putbuf()` (at least for moderate sizes), to avoid interleaved output from
 *   concurrent processes. For very large buffers (hundreds of bytes or more),
 *   breaking into smaller chunks is acceptable.
 *
 * This behavior ensures consistent and readable console output and supports
 * basic file write semantics under the constraints of the file system.
 */
int write(int fd_num, const void *buffer, unsigned size)
{
	if (buffer == NULL || !is_user_vaddr(buffer))
		return -1;

	if (fd_num == STDOUT_FILENO)
	{
		putbuf(buffer, size);
		return size;
	}

	if (fd_num == STDIN_FILENO)
		return -1;

	struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
	if (fd == NULL)
		return -1;


	int bytes_written = file_write(fd->file, buffer, size);


	return bytes_written;
}

/**
 * Sets the next byte to be read or written in the open file `fd` to the specified
 * `position`, given in bytes from the beginning of the file. For example, a
 * position of 0 indicates the start of the file.
 *
 * Seeking past the current end-of-file (EOF) is allowed and does not produce
 * an error. A subsequent read from this position will return 0 bytes, indicating
 * EOF. A subsequent write would normally extend the file, filling any unwritten
 * gap with zeros.
 *
 * However, in the Pintos file system (until project 4 is complete), file lengths
 * are fixed. As a result, writes beyond the current end of the file will fail,
 * and no file growth will occur.
 *
 * These seek semantics are implemented within the file system and do not require
 * special handling in the system call layer.
 */
void seek(int fd_num, unsigned position)
{
	struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
	if (fd == NULL)
		return;

	file_seek(fd->file, position);
}

/**
 * Returns the current position of the file pointer for the open file `fd`.
 * The position is expressed as the number of bytes from the beginning of the file
 * to the next byte that will be read or written.
 */
unsigned tell(int fd_num)
{
	struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
	if (fd == NULL)
		return -1;

	unsigned position = file_tell(fd->file);

	return position;
}

/**
 * Closes the file associated with the given file descriptor `fd`.
 * If the process exits or is terminated, all its open file descriptors
 * are automatically closed as if this function were called on each one.
 */
void close(int fd_num)
{
	if (fd_num == STDIN_FILENO || fd_num == STDOUT_FILENO)
		return;
	struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
	if (fd == NULL)
		return;

	file_close(fd->file);

	list_remove(&fd->elem);
	free(fd);
}

/**
 * Retrieves the file descriptor structure associated with a given file descriptor `fd_num` for the specified thread `t`.
 * Searches the thread's list of file descriptors to find the one matching `fd_num`.
 * If the file descriptor is not found, the function returns NULL.
 */
struct file_descriptor *file_get_fd(int fd_num, struct thread *thread)
{
	if (fd_num < 0 || fd_num == STDIN_FILENO || fd_num == STDOUT_FILENO || thread == NULL)
		return NULL;

	for (struct list_elem *e = list_begin(&thread->file_descriptors); e != list_end(&thread->file_descriptors); e = list_next(e))
	{
		struct file_descriptor *fd = list_entry(e, struct file_descriptor, elem);
		if (fd->fd == fd_num)
			return fd;
	}

	return NULL;
}

/**
 * Adds the given file `file` to the file descriptor list of the current thread `thread`.
 * A new file descriptor (fd) is allocated and assigned to the file. The file descriptor
 * is then pushed onto the thread's list of file descriptors.
 *
 * Returns the newly assigned file descriptor (fd) if successful, or -1 if an error occurs.
 *
 * If `f` is NULL or memory allocation for the file descriptor fails, the function returns -1.
 */
int file_add_to_thread(struct file *file, struct thread *thread)
{
	if (file == NULL || thread == NULL)
		return INVALID_FILE;

	struct file_descriptor *fd = malloc(sizeof(struct file_descriptor));
	if (fd == NULL)
		return INVALID_FD;

	fd->file = file;
	fd->fd = thread->next_fd;

	thread->next_fd++;
	list_push_back(&thread->file_descriptors, &fd->elem);

	return fd->fd;
}
int wait(tid_t e) { return process_wait(e); }

tid_t exec(const char *cmd_line)
{
	int ret;
	ret = process_execute(cmd_line);
	return ret;
}
