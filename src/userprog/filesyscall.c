#include "userprog/filesyscall.h"

#include <stdio.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "userprog/syscall.h"

/**
 * Creates a new file with the given name and initial size.
 * Returns true on success, false otherwise.
 * Does not open the file.
 */
bool file_create(const char *file_name, unsigned initial_size)
{
    return file_name != NULL && filesys_create(file_name, initial_size);
}

/**
 * Deletes the file called file. Returns true if successful, false otherwise.
 * A file may be removed regardless of whether it is open or closed.
 */
bool file_remove(const char *file_name)
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
int file_open(const char *file_name)
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
int file_size_fd(int fd_num)
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
int file_read_fd(int fd_num, void *buffer, unsigned size)
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
int file_write_fd(int fd_num, const void *buffer, unsigned size)
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
void file_seek_fd(int fd_num, unsigned position)
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
unsigned file_tell_fd(int fd_num)
{
    struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
    if (fd == NULL) return (unsigned)-1;

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
void file_close_fd(int fd_num)
{
    struct file_descriptor *fd = file_get_fd(fd_num, thread_current());
    if (fd == NULL) return;

    lock_acquire(&fd->file->file_lock);
    file_close(fd->file);
    lock_release(&fd->file->file_lock);

    list_remove(&fd->elem);
    free(fd);
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
void file_close_all(struct thread *thread)
{
    if (thread == NULL) return;
    struct list_elem *e = list_begin(&thread->file_descriptors);
    while (e != list_end(&thread->file_descriptors))
    {
        struct file_descriptor *fd = list_entry(e, struct file_descriptor, elem);
        e = list_next(e);
        file_close_fd(fd->fd);
    }
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