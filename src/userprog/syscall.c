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
// #include "userprog/filesyscall.h"
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
		f->eax = create(args[0], args[1]);
		break;
	case SYS_REMOVE:
		conv_virtual(args[0]);
		f->eax = remove(args[0]);
		break;
	case SYS_OPEN:
		// printf("%s", args[0]);
		conv_virtual(args[0]);
		f->eax = open(args[0]);
		break;
	case SYS_FILESIZE:
		f->eax = filesize(args[0]);
		break;
	case SYS_READ:
		// printf("-------- %ud -------------\n", args[2]);
		f->eax = read(args[0], args[1], args[2]);
		break;
	case SYS_WRITE:
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
/**
 * Closes all open file descriptors associated with the given thread `t`.
 *
 * This function is typically called when a thread exits or is terminated,
 * ensuring that all files opened by the thread are properly closed and
 * associated resources are released. It iterates through the thread's
 * list of open file descriptors, closes each file, removes the descriptor
 * from the list, and frees its memory.
 */
// void close_all(struct thread *t)
// {
//     if (t == NULL) return;
//     struct list_elem *e = list_begin(&t->file_descriptors);
//     while (e != list_end(&t->file_descriptors))
//     {
//         struct file_descriptor *fd = list_entry(e, struct file_descriptor, elem);
//         e = list_next(e);
//         close (fd->fd);
//     }
// }

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
    struct list_elem *e = list_begin(&cur->file_descriptors);
    while (e != list_end(&cur->file_descriptors))
    {
        struct file_descriptor *fd = list_entry(e, struct file_descriptor, elem);
        e = list_next(e);
        close (fd->fd);
    }
	thread_exit();
}

void handle_halt(void) { 
	shutdown_power_off();
}

tid_t exec(const char *cmd_line) {
	return process_execute(cmd_line);
}

int wait(tid_t child) { return process_wait(child); }


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
    if (file_name == NULL) return INVALID_FILE;

    lock_acquire(&filesys_lock);
    struct file *file = filesys_open(file_name);
    lock_release(&filesys_lock);

    if (file == NULL) return INVALID_FILE;

    int fd = file_add_to_thread(file, thread_current());

    if(fd == INVALID_FD)
    {
        lock_acquire(&filesys_lock);
        file_close(file);
        lock_release(&filesys_lock);
    }

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
int filesize (int fd_num)
{
    struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
    if (fd == NULL) return -1;
    
    lock_acquire(&fd->file->file_lock);
    int size = file_length(fd->file);
    lock_release(&fd->file->file_lock);

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
    if (buffer == NULL || !is_user_vaddr(buffer)) return -1;

    if (fd_num == STDIN_FILENO)
    {
        uint8_t *buf = buffer;
        lock_acquire(&std_input_lock);
        for (unsigned i = 0; i < size; i++)
        {
            buf[i] = input_getc();
        }
        lock_release(&std_input_lock);
        return size;
    }

    if (fd_num == STDOUT_FILENO) return -1;

    struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
    if (fd == NULL) return -1;

    lock_acquire(&fd->file->file_lock);
    int bytes_read = file_read(fd->file, buffer, size);
    lock_release(&fd->file->file_lock);

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
    if (buffer == NULL || !is_user_vaddr(buffer)) return -1;
    
    if (fd_num == STDOUT_FILENO)
    {
        lock_acquire(&std_output_lock);
        putbuf(buffer, size);
        lock_release(&std_output_lock);
        return size;
    }
    
    if (fd_num == STDIN_FILENO) return -1;
    
    struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
    if (fd == NULL) return -1;

    lock_acquire(&fd->file->file_lock);
    int bytes_written = file_write(fd->file, buffer, size);
    lock_release(&fd->file->file_lock);

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
    if (fd == NULL) return;

    lock_acquire(&fd->file->file_lock);
    file_seek(fd->file, position);
    lock_release(&fd->file->file_lock);
}

/**
 * Returns the current position of the file pointer for the open file `fd`.
 * The position is expressed as the number of bytes from the beginning of the file
 * to the next byte that will be read or written.
 */
unsigned tell(int fd_num)
{
    struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
    if (fd == NULL) return -1;

    lock_acquire(&fd->file->file_lock);
    unsigned position = file_tell(fd->file);
    lock_release(&fd->file->file_lock);

    return position;
}

/**
 * Closes the file associated with the given file descriptor `fd`.
 * If the process exits or is terminated, all its open file descriptors
 * are automatically closed as if this function were called on each one.
 */
void close(int fd_num)
{
    if (fd_num == STDIN_FILENO || fd_num == STDOUT_FILENO) return;
    struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
    if (fd == NULL) return;

    lock_acquire(&fd->file->file_lock);
    file_close(fd->file);
    lock_release(&fd->file->file_lock);

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
    if (fd_num < 0 || fd_num == STDIN_FILENO || fd_num == STDOUT_FILENO || thread == NULL) return NULL;

    for (struct list_elem *e = list_begin(&thread->file_descriptors); e != list_end(&thread->file_descriptors); e = list_next(e))
    {
        struct file_descriptor *fd = list_entry(e, struct file_descriptor, elem);
        if (fd->fd == fd_num) return fd;
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
    if (file == NULL || thread == NULL) return INVALID_FILE;

    struct file_descriptor *fd = malloc(sizeof(struct file_descriptor));
    if (fd == NULL) return INVALID_FD;

    lock_acquire(&filesys_lock);
    fd->file = file;
    fd->fd = thread->next_fd++;
    list_push_back(&thread->file_descriptors, &fd->elem);
    lock_release(&filesys_lock);

    return fd->fd;
}