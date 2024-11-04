#include "console.h"
#include "thread.h"
#include "device.h"
#include "uart.h"
#include "timer.h"
#include "intr.h"
#include "heap.h"
#include "halt.h"
#include "elf.h"
#include "fs.h"
#include "string.h"

extern char _kimg_end[];

#define RAM_SIZE (8 * 1024 * 1024)
#define RAM_START 0x80000000UL
#define KERN_START RAM_START
#define USER_START 0x80100000UL

#define UART0_IOBASE 0x10000000
#define UART1_IOBASE 0x10000100
#define UART0_IRQNO 10

#define VIRT0_IOBASE 0x10001000
#define VIRT1_IOBASE 0x10002000
#define VIRT0_IRQNO 1

extern char _companion_f_start[];
extern char _companion_f_end[];

static void shell_main(struct io_intf *termio);

int main()
{
  struct io_intf *termio;
  int result;
  void *mmio_base;
  int i;
  console_init();
  devmgr_init();
  intr_init();
  thread_init();
  timer_init();

  heap_init(_kimg_end, (void *)USER_START);

  for (i = 0; i < 2; i++)
  {
    mmio_base = (void *)UART0_IOBASE;
    mmio_base += (UART1_IOBASE - UART0_IOBASE) * i;
    uart_attach(mmio_base, UART0_IRQNO + i);
  }

  size_t total_size = _companion_f_end - _companion_f_start;

  intr_enable();
  timer_start();

  struct io_lit lit_dev;
  struct io_intf *lit_dev_intf = NULL;
  lit_dev_intf = iolit_init(&lit_dev, _companion_f_start, total_size);

  result = fs_mount(lit_dev_intf);

  debug("Mounted lit_dev");

  if (result != 0)
    panic("fs_mount failed");

  result = device_open(&termio, "ser", 1);
  if (result != 0)
    panic("Could not open ser1");

  shell_main(termio);
}

void shell_main(struct io_intf *termio_raw)
{
  struct io_term ioterm;
  struct io_intf *termio;
  void (*exe_entry)(struct io_intf *);
  struct io_intf *exeio;
  char cmdbuf[9];
  int tid;
  int result;

  termio = ioterm_init(&ioterm, termio_raw);

  ioputs(termio, "Welcome to the companion shell\n");

  for (;;)
  {
    ioprintf(termio, "companion_sh$> ");

    ioterm_getsn(&ioterm, cmdbuf, sizeof(cmdbuf));
    if (cmdbuf[0] == '\0')
      continue;
    if (strcmp(cmdbuf, "exit") == 0)
    {
      return;
    }
    result = fs_open(cmdbuf, &exeio);
    if (result < 0)
    {
      if (result == -ENOENT)
      {
        ioprintf(termio, "%s: E:file not found\n", cmdbuf);
      }
      else
      {
        ioprintf(termio, "%s: E:unknown error with code %d\n", cmdbuf, result);
      }
      continue;
    }
    console_printf("exeio: %p\n", exeio);
    debug("Calling elf_load(\"%s\")", cmdbuf);

    result = elf_load(exeio, &exe_entry);

    debug("elf_load returned %d", result);
    console_printf("result: %d\n", result);
    if (result < 0)
    {
      ioprintf(termio, "%s: Error %d\n", -result);
    }
    else
    {
      console_printf("exe_entry: %p\n", exe_entry);
      console_printf("spawn thread\n");
      tid = thread_spawn(cmdbuf, (void *)exe_entry, termio_raw);

      if (tid < 0)
        ioprintf(termio, "%s: Error %d\n", -result);
      else
        console_printf("spawned thread %d\n", tid);
      thread_join(tid);
    }

    ioclose(exeio);
  }
}