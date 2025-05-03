```c
pid_t exec (const char *cmd_line)
```

**\[System Call]**
Runs the executable whose name is given in `cmd_line`, passing any given arguments, and returns the new processs program id (pid).
Must return `pid -1`, which otherwise should not be a valid pid, if the program cannot load or run for any reason.
Thus, the parent process cannot return from the exec until it knows whether the child process successfully loaded its executable.
You must use appropriate synchronization to ensure this.

---

```c
int wait (pid_t pid)
```

Waits for a child process `pid` and retrieves the childs exit status.
**\[System Call]**
If `pid` is still alive, waits until it terminates. Then, returns the status that `pid` passed to `exit`.
If `pid` did not call `exit()`, but was terminated by the kernel (e.g. killed due to an exception), `wait(pid)` must return `-1`.
It is perfectly legal for a parent process to wait for child processes that have already terminated by the time the parent calls `wait`,
but the kernel must still allow the parent to retrieve its childs exit status, or learn that the child was terminated by the kernel.

---

`wait` must fail and return `-1` immediately if any of the following conditions is true:

* `pid` does not refer to a direct child of the calling process. `pid` is a direct child of the calling process if and only if the calling process received `pid` as a return value from a successful call to `exec`.
* Note that children are not inherited:
  if A spawns child B and B spawns child process C, then A cannot wait for C, even if B is dead.
  A call to `wait(C)` by process A must fail.
  Similarly, orphaned processes are not assigned to a new parent if their parent process exits before they do.
* The process that calls `wait` has already called `wait` on `pid`. That is, a process may wait for any given child at most once.

---

Processes may spawn any number of children, wait for them in any order, and may even exit without having waited for some or all of their children.
Your design should consider all the ways in which waits can occur.
All of a processs resources, including its `struct thread`, must be freed whether its parent ever waits for it or not,
and regardless of whether the child exits before or after its parent.

You must ensure that Pintos does not terminate until the initial process exits.
The supplied Pintos code tries to do this by calling `process_wait()` (in `userprog/process.c`) from `main()` (in `threads/init.c`).
We suggest that you implement `process_wait()` according to the comment at the top of the function
and then implement the `wait` system call in terms of `process_wait()`.

> Implementing this system call requires considerably more work than any of the rest.

