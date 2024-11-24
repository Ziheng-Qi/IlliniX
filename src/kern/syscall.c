#include <stdint.h>
#include <stddef.h>
#include "trap.h"
#include "scnum.h"
#include "console.h"
#include "device.h"
#include "process.h"
#include "error.h"
#include "fs.h"
#include "memory.h"
/*
 * syscall will be used for requesting actions from the kernel
 */
// RV64I syscall instruction
// These functions are for handling the system calls after the handler is called
static int sysexit(void)
{
  // Exit the current process
  process_exit();
  kprintf("Code should not reach here.");
  return 0;
}

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
  proc->iotab[fd] = NULL;
  
  return 0;
}

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
  return ioread(io, buf, bufsz);
}

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
  return iowrite(io, buf, len);
}

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
  return ioctl(io, cmd, arg);
}

static int sysdevopen(int fd, const char *name, int instno)
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

  int result = device_open(&(proc->iotab[fd]), name, instno);
  if (result < 0)
  {
    return result;
  }

  return 0;
}

static int sysfsopen(int fd, const char *name)
{
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
  struct process *proc = current_process();
  if (proc == NULL)
  {
    return -ENOENT;
  }
  if (fd < 0 || fd >= MAX_FILE_OPEN)
  {
    return -EBADFD;
  }
  proc->iotab[fd] = io;
  return 0;
}

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

/*
int sysfork(struct trap_frame *tfr)
{
  // Fork the current process
  struct process *proc = current_process();
  if (proc == NULL)
  {
    return -ENOENT;
  }
  struct process *new_proc = memory_fork(proc);

}
*/
void syscall_handler(struct trap_frame *tfr)
{
  // Get values within register a7 to determine which syscall to call
  tfr->sepc += 4;
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
  /*
  case SYSCALL_FORK:
    tfr->x[TFR_A0] = sysfork(tfr);
    break;
  */
  default:
    break;
  }
}