#include "fs.h"
#include "virtio.h"
#include "console.h"

extern char _companion_f_start[];
extern char _comapanion_f_end[];

int main()
{
  // Initialize the io_lit structure for unit testing
  struct io_lit literature;
  struct io_intf *io = NULL;
  io = iolit_init(&literature, _companion_f_start, BLOCK_SIZE * 7);
  // Initialize the file system
  // fs_init();
  // Mount the block device
  if (io->ops->read == NULL)
  {
    console_printf("read is NULL\n");
  }

  fs_mount(io);
  // Open a file
  struct io_intf *file_io = NULL;
  fs_open("helloworld.txt", &file_io);
  struct io_intf *file_io2 = NULL;
  fs_open("test_input.txt", &file_io2);
  char buf2[8926];

  console_putchar('\n');
  console_putchar('\n');

  fs_read(file_io2, buf2, 8926);
  for (int i = 0; i < 8926; i++)
  {
    // console_putchar(buf2[i]);
  }
  char buf[435];
  fs_read(file_io, buf, 435);

  console_putchar('\n');
  console_putchar('\n');

  for (int i = 0; i < 435; i++)
  {
    // console_putchar(buf[i]);
  }

  console_putchar('\n');
  console_putchar('\n');
  console_printf("Current position1: %d\n", fs_ioctl(file_io2, IOCTL_GETPOS, NULL));
  fs_ioctl(file_io2, IOCTL_SETPOS, 0);
  console_printf("Current position2: %d\n", fs_ioctl(file_io2, IOCTL_GETPOS, NULL));
  fs_write(file_io2, buf, 435);
  console_printf("Current position3: %d\n", fs_ioctl(file_io2, IOCTL_GETPOS, NULL));
  fs_ioctl(file_io2, IOCTL_SETPOS, 0);
  console_printf("Current position4: %d\n", fs_ioctl(file_io2, IOCTL_GETPOS, NULL));
  char buf3[8926];
  fs_read(file_io2, buf3, 8926);

  console_putchar('\n');
  console_putchar('\n');

  for (int i = 0; i < 8926; i++)
  {
    console_putchar(buf3[i]);
  }

  return 0;
}