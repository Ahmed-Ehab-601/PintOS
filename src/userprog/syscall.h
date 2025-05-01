#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void* conv_virtual (void * esp);
int get_number_of_args(int systemCall);
void load_args(int *args, int numberOfArgs, int * esp);
void exit(int status);
void halt(void);

#endif /* userprog/syscall.h */
