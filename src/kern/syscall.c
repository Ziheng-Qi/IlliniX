#include <stdint.h>
#include <stddef.h>
#include "trap.h"
#include "scnum.h"
#include "console.h"
#include "device.h"
#include "process.h"
#include "error.h"
#include "fs.h"
#include "timer.h"
#include "memory.h"

#define PC_ALIGN 4
/*
 * syscall will be used for requesting actions from the kernel
 */
// RV64I syscall instruction
// These functions are for handling the system calls after the handler is called
/**
 * @brief Exits the current process.
 *
 * This function is responsible for terminating the current process
 * by calling the process_exit() function. It should not return
 * control to the caller, as indicated by the debug message.
 *
 * @return This function always returns 0, although it should not
 *         reach this point.
 */
static int sysexit(void)
{
  // Exit the current process
  process_exit();
  kprintf("Code should not reach here.");
  return 0;
}

/**
 * @brief Outputs a system message to the kernel console.
 *
 * This function validates the provided message string and then prints it
 * to the kernel console along with the name and ID of the currently running thread.
 *
 * @param msg The message string to be printed. It must be a valid user-space string.
 * @return 0 on success, or a non-zero error code if the message string is invalid.
 */
static int sysmsgout(const char *msg)
{
  int result;

  trace("%s(msg=%p)\n", __func__, msg);
  result = memory_validate_vstr(msg, PTE_U);
  if (result != 0)
    return result;
  kprintf("Thread <%s:%d> says: %s\n",
          thread_name(running_thread()),
          running_thread(), msg);
  return 0;
}

/**
 * @brief Closes the device at the specified file descriptor.
 *
 * This function attempts to close the device associated with the given file descriptor.
 * It performs several checks to ensure the validity of the file descriptor and the
 * current process before closing the device.
 *
 * @param fd The file descriptor of the device to close.
 * @return 0 on success, or a negative error code on failure:
 *         -EBADFD: if the file descriptor is invalid or not open.
 *         -ENOENT: if the current process is not found.
 */
static int sysclose(int fd)
{
  // close the device at the specified file descriptor
  if (fd < 0 || fd >= MAX_FILE_OPEN)
  {
    return -EBADFD;
  }
  struct process *proc = current_process();
  if (proc == NULL)
  {
    return -ENOENT;
  }
  if (proc->iotab[fd] == NULL)
  {
    return -EBADFD;
  }
  struct io_intf *io = proc->iotab[fd];
  ioclose(io);
  return 0;
}

/**
 * @brief Reads data from a device associated with a file descriptor.
 *
 * This function attempts to read data from the device specified by the file
 * descriptor `fd` into the buffer `buf` of size `bufsz`.
 *
 * @param fd The file descriptor from which to read.
 * @param buf The buffer where the read data will be stored.
 * @param bufsz The size of the buffer.
 * @return The number of bytes read on success, or a negative error code on failure.
 *         Possible error codes include:
 *         - -EBADFD: Invalid file descriptor.
 *         - -ENOENT: No current process.
 */
static int sysread(int fd, void *buf, size_t bufsz)
{
  // read from the device at the specified file descriptor
  if (fd < 0 || fd >= MAX_FILE_OPEN)
  {
    return -EBADFD;
  }
  struct process *proc = current_process();
  if (proc == NULL)
  {
    return -ENOENT;
  }
  if (proc->iotab[fd] == NULL)
  {
    return -EBADFD;
  }
  struct io_intf *io = proc->iotab[fd];
  int result = memory_validate_vptr_len(buf, bufsz, PTE_U | PTE_R);
  if (result != 0)
  {
    return result;
  }
  return ioread(io, buf, bufsz);
}

/**
 * @brief Writes data to a file descriptor.
 *
 * This function attempts to write `len` bytes from the buffer `buf` to the file
 * descriptor `fd`. It performs several checks to ensure the validity of the file
 * descriptor and the current process.
 *
 * @param fd The file descriptor to write to.
 * @param buf A pointer to the buffer containing the data to write.
 * @param len The number of bytes to write from the buffer.
 * @return On success, the number of bytes written is returned. On error, a negative
 *         error code is returned:
 *         - -EBADFD: The file descriptor is invalid.
 *         - -ENOENT: The current process is not found.
 */
static int syswrite(int fd, const void *buf, size_t len)
{
  if (fd < 0 || fd >= MAX_FILE_OPEN)
  {
    return -EBADFD;
  }
  struct process *proc = current_process();
  if (proc == NULL)
  {
    return -ENOENT;
  }
  if (proc->iotab[fd] == NULL)
  {
    return -EBADFD;
  }
  struct io_intf *io = proc->iotab[fd];
  // kprintf("io %p\n", io);
  int result = memory_validate_vptr_len(buf, len, PTE_U | PTE_W);
  if (result != 0)
  {
    return result;
  }
  return iowrite(io, buf, len);
}

/**
 * @brief Perform an ioctl operation on a file descriptor.
 *
 * This function performs an ioctl (input/output control) operation on a given
 * file descriptor. It checks the validity of the file descriptor and the
 * current process, and then delegates the ioctl operation to the appropriate
 * I/O interface.
 *
 * @param fd The file descriptor on which to perform the ioctl operation.
 * @param cmd The ioctl command to execute.
 * @param arg A pointer to the argument for the ioctl command.
 * @return 0 on success, or a negative error code on failure.
 *         - -EBADFD: if the file descriptor is invalid.
 *         - -ENOENT: if the current process is not found.
 */
static int sysioctl(int fd, const int cmd, void *arg)
{
  if (fd < 0 || fd >= MAX_FILE_OPEN)
  {
    return -EBADFD;
  }
  struct process *proc = current_process();
  if (proc == NULL)
  {
    return -ENOENT;
  }
  if (proc->iotab[fd] == NULL)
  {
    return -EBADFD;
  }
  struct io_intf *io = proc->iotab[fd];
  // kprintf("io at ioctl %p\n", io);
  int result = ioctl(io, cmd, arg);

  return result;
}

/**
 * @brief Opens a device and associates it with a file descriptor in the current process.
 *
 * This function attempts to open a device specified by its name and instance number,
 * and associates it with the given file descriptor in the current process's I/O table.
 *
 * @param fd The file descriptor to associate with the device. Must be within the valid range.
 * @param name The name of the device to open.
 * @param instno The instance number of the device to open.
 * @return 0 on success, or a negative error code on failure:
 *         - -ENOENT: if the current process is NULL.
 *         - -EBADFD: if the file descriptor is out of range.
 *         - A negative value returned by device_open() if the device could not be opened.
 */
static int sysdevopen(int fd, const char *name, int instno)
{
  struct process *proc = current_process();

  if (proc == NULL)
  {
    return -ENOENT;
  }

  if (fd >= MAX_FILE_OPEN)
  {
    return -EBADFD;
  }

  if (fd < 0)
  {
    // find the next empty entry of proc->iotab
    for (int i = 0; i < MAX_FILE_OPEN; i++)
    {
      if (proc->iotab[i] == NULL)
      {
        fd = i;
        break;
      }
    }
  }
  if (proc->iotab[fd] != NULL)
  {
    ioref(proc->iotab[fd]);
    return fd;
  }
  int result = device_open(&(proc->iotab[fd]), name, instno);
  if (result < 0)
  {
    return result;
  }

  return fd;
}

/**
 * @brief Opens a file and associates it with a file descriptor in the current process.
 *
 * This function attempts to open a file specified by the given name and associates
 * it with the provided file descriptor (fd) in the current process's I/O table.
 *
 * @param fd The file descriptor to associate with the opened file. Must be within
 *           the valid range [0, MAX_FILE_OPEN).
 * @param name The name of the file to open.
 * @return 0 on success, or a negative error code on failure:
 *         - -ENODEV: if the I/O interface is NULL.
 *         - -ENOENT: if the current process is NULL.
 *         - -EBADFD: if the file descriptor is out of range.
 *         - Other negative values returned by fs_open() on failure.
 */
static int sysfsopen(int fd, const char *name)
{

  struct process *proc = current_process();
  if (proc == NULL)
  {
    return -ENOENT;
  }
  if (fd >= MAX_FILE_OPEN)
  {
    return -EBADFD;
  }
  if (fd < 0)
  {
    // find the next empty entry of proc->iotab
    for (int i = 0; i < MAX_FILE_OPEN; i++)
    {
      if (proc->iotab[i] == NULL)
      {
        fd = i;
        break;
      }
    }
  }
  if (proc->iotab[fd] != NULL)
  {
    ioref(proc->iotab[fd]);
    kprintf("File already opened\n");
    return fd;
  }

  struct io_intf *io;
  int result = fs_open(name, &io);
  if (result < 0)
  {
    return result;
  }
  if (io == NULL)
  {
    return -ENODEV;
  }
  proc->iotab[fd] = io;

  return fd;
}

/**
 * @brief Executes a process from a file descriptor.
 *
 * This function attempts to execute a process using the file descriptor provided.
 * It performs several checks to ensure the validity of the current process and the file descriptor.
 * If the checks pass, it calls `process_exec` to execute the process.
 *
 * @param fd The file descriptor of the executable to run.
 * @return 0 on success, or a negative error code on failure:
 *         - -ENOENT: if the current process is NULL.
 *         - -EBADFD: if the file descriptor is invalid or the I/O interface is NULL.
 *         - Any negative value returned by `process_exec` if execution fails.
 */
static int sysexec(int fd)
{
  struct process *proc = current_process();
  if (proc == NULL)
  {
    return -ENOENT;
  }
  if (fd < 0 || fd >= MAX_FILE_OPEN)
  {
    return -EBADFD;
  }

  struct io_intf *io = proc->iotab[fd];

  if (io == NULL)
  {
    return -EBADFD;
  }

  int result = process_exec(io);
  if (result < 0)
  {
    return result;
  }

  return 0;
}

/**
 * @brief Waits for a thread to finish execution.
 *
 * This function waits for a specific thread to complete its execution.
 * If the thread ID (tid) is 0, it waits for any thread to finish.
 * Otherwise, it waits for the thread with the specified ID.
 *
 * @param tid The thread ID to wait for. If 0, waits for any thread.
 * @return Returns the result of thread_join or thread_join_any.
 */

static int syswait(int tid)
{
  trace("%s(%d)", __func__, tid);
  if (tid == 0)
    return thread_join_any();
  else
    return thread_join(tid);
}

/**
 * @brief Suspends the execution of the current thread for a specified number of microseconds.
 *
 * This function puts the current thread to sleep for the specified duration in microseconds.
 * It returns 0 on success, or a negative error code on failure.
 *
 * @param us The number of microseconds to sleep.
 * @return 0 on success, or a negative error code on failure.
 */
static int sysusleep(unsigned long us)
{
  // sleep for a certain amount of time
  // us is the number of microseconds to sleep
  // return 0 on success, or a negative error code on failure

  // Check if the current process is NULL
  struct process *proc = current_process();
  if (proc == NULL)
  {
    return -ENOENT;
  }

  // suspend the current thread of us microseconds
  struct alarm *alarm = kmalloc(sizeof(struct alarm));
  alarm_init(alarm, "usleep");
  alarm_sleep_us(alarm, us);
  return 0;
}

/**
 * @brief Forks the current process.
 *
 * This function creates a new process by duplicating the current process.
 * It retrieves the current process and checks if it is valid. If the current
 * process is valid, it proceeds to fork the process using the provided trap frame.
 *
 * @param tfr Pointer to the trap frame containing the CPU state at the time of the fork.
 * @return On success, returns the process ID of the child process. On failure, returns -ENOENT if the current process is NULL.
 */
static int sysfork(struct trap_frame *tfr)
{
  // Fork the current process
  struct process *proc = current_process();
  if (proc == NULL)
  {
    return -ENOENT;
  }
  
  return process_fork(tfr);
}

/**
 * @brief Handles system calls by dispatching to the appropriate syscall function.
 *
 * This function is called when a system call is triggered. It inspects the value
 * in the register a7 (stored in tfr->x[TFR_A7]) to determine which system call
 * to execute. The program counter (sepc) is incremented by 4 to move past the
 * syscall instruction.
 *
 * @param tfr Pointer to the trap frame containing the state of the registers.
 *
 * The following system calls are handled:
 * - SYSCALL_EXIT: Terminates the calling process.
 * - SYSCALL_MSGOUT: Outputs a message.
 * - SYSCALL_CLOSE: Closes a file descriptor.
 * - SYSCALL_READ: Reads from a file descriptor.
 * - SYSCALL_WRITE: Writes to a file descriptor.
 * - SYSCALL_IOCTL: Performs an I/O control operation.
 * - SYSCALL_DEVOPEN: Opens a device.
 * - SYSCALL_FSOPEN: Opens a file system.
 * - SYSCALL_EXEC: Executes a new program.
 * - SYSCALL_FORK: Forks the current process.
 * - SYSCALL_USLEEP: Sleeps for a specified number of microseconds.
 * - SYSCALL_WAIT: Waits for a child process to exit.
 * If the syscall number does not match any of the handled cases, the function
 * does nothing.
 */
void syscall_handler(struct trap_frame *tfr)
{
  // Get values within register a7 to determine which syscall to call
  tfr->sepc += PC_ALIGN;
  switch (tfr->x[TFR_A7])
  {
  case SYSCALL_EXIT:
    sysexit();
    break;
  case SYSCALL_MSGOUT:
    tfr->x[TFR_A0] = sysmsgout((const char *)tfr->x[TFR_A0]);
    break;
  case SYSCALL_CLOSE:
    tfr->x[TFR_A0] = sysclose((int)tfr->x[TFR_A0]);
    break;
  case SYSCALL_READ:
    tfr->x[TFR_A0] = sysread((int)tfr->x[TFR_A0], (void *)tfr->x[TFR_A1], (size_t)tfr->x[TFR_A2]);
    break;
  case SYSCALL_WRITE:
    tfr->x[TFR_A0] = syswrite((int)tfr->x[TFR_A0], (const void *)tfr->x[TFR_A1], (size_t)tfr->x[TFR_A2]);
    break;
  case SYSCALL_IOCTL:
    tfr->x[TFR_A0] = sysioctl((int)tfr->x[TFR_A0], (const int)tfr->x[TFR_A1], (void *)tfr->x[TFR_A2]);
    break;
  case SYSCALL_DEVOPEN:
    tfr->x[TFR_A0] = sysdevopen((int)tfr->x[TFR_A0], (const char *)tfr->x[TFR_A1], (int)tfr->x[TFR_A2]);
    break;
  case SYSCALL_FSOPEN:
    tfr->x[TFR_A0] = sysfsopen((int)tfr->x[TFR_A0], (const char *)tfr->x[TFR_A1]);
    break;
  case SYSCALL_EXEC:
    tfr->x[TFR_A0] = sysexec((int)tfr->x[TFR_A0]);
    break;
  
  case SYSCALL_FORK:
    tfr->x[TFR_A0] = sysfork(tfr);
    break;

  case SYSCALL_WAIT:
    tfr->x[TFR_A0] = syswait((int)tfr->x[TFR_A0]);
    break;
  case SYSCALL_USLEEP:
    tfr->x[TFR_A0] = sysusleep((unsigned long)tfr->x[TFR_A0]);
    break;
  default:
    break;
  }
}