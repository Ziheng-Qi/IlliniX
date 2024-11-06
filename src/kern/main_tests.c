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

// static void shell_main(struct io_intf *termio);

int main()
{
  int result = 0;
  console_init();
  intr_init();
  devmgr_init();
  thread_init();
  timer_init();
  heap_init(_kimg_end, (void *)USER_START);
  for (int i = 0; i < 8; i++)
  {
    void *mmio_base = (void *)VIRT0_IOBASE;
    mmio_base += (VIRT1_IOBASE - VIRT0_IOBASE) * i;
    virtio_attach(mmio_base, VIRT0_IRQNO + i);
  }
  intr_enable();
  timer_start();
  struct io_intf *blkio = NULL;
  result = device_open(&blkio, "blk", 0);
  if (result < 0)
  {
    halt_failure();
  }
  // struct io_lit *lit_dev;
  // struct io_intf *lit_io;
  // lit_io = iolit_init(&lit_dev, _companion_f_start, _companion_f_end - _companion_f_start);
  fs_mount(blkio);
  struct io_intf *file_io;
  result = fs_open("helloworld.txt", &file_io);
  if (result < 0)
  {
    halt_failure();
  }
  size_t file_size = 0;
  result = ioctl(file_io, IOCTL_GETLEN, &file_size);
  if (result < 0)
  {
    halt_failure();
  }
  char buf[file_size + 1];
  size_t pos = 0;
  result = ioseek(file_io, pos);
  if (result < 0)
  {
    halt_failure();
  }
  result = ioread_full(file_io, &buf, file_size);
  if (result < 0)
  {
    halt_failure();
  }
  buf[file_size] = '\0';
  kprintf("\n\n\n\n");
  kprintf("Printing the file\n");
  for (int i = 0; i < file_size; i++)
  {
    console_putchar(buf[i]);
  }

  result = ioseek(file_io, 0);
  if (result < 0)
  {
    halt_failure();
  }

  char data[] = "Changed everything and the ultimate secret is 42";
  result = iowrite(file_io, &data, sizeof(data));
  if (result < 0)
  {
    halt_failure();
  }
  kprintf("\n\n\n\n");
  pos = 0;
  struct io_intf *file_io2;
  result = fs_open("helloworld.txt", &file_io2);
  if (result < 0)
  {
    halt_failure();
  }
  result = ioseek(file_io2, 0);
  if (result < 0)
  {
    halt_failure();
  }
  char buf2[file_size + 1];
  kprintf("file size: %d\n", file_size);
  result = ioread_full(file_io2, &buf2, file_size);
  if (result < 0)
  {
    halt_failure();
  }
  buf2[file_size] = '\0';
  kprintf("Printing the file after writing\n");
  for (int i = 0; i < file_size; i++)
  {
    console_putchar(buf2[i]);
  }
  return 0;
}