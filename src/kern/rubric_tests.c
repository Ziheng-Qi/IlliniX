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

/*
 * kfs.raw info
 * Making fs
 * File name is helloworld.txt
 * Dentry index is 0
 * Inode number is 0
 * File name is trek
 * Dentry index is 1
 * Inode number is 1
 * File name is enum.txt
 * Dentry index is 2
 * Inode number is 2
 * Number of bytes for file helloworld.txt: 435
 * Number of bytes for file trek: 45280
 * Number of bytes for file enum.txt: 1098
 * Total number of dentries: 3
 * Total number of inodes: 3
 * Total number of data blocks: 14
 * Wrote Inode 0, Program: helloworld.txt
 * Wrote Inode 1, Program: trek
 * Wrote Inode 2, Program: enum.txt
 * Wrote filesystem image to kfs.raw
 */

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
    // attached
    assert(mmio_base != NULL);
  }
  intr_enable();
  timer_start();

  if (result < 0)
  {
    halt_failure();
  }
  struct io_lit lit_dev;
  struct io_intf *lit_io = NULL;

  /*
   * Here starts the test for io_lit device FULL I/O
   */

  char buf[] = "Hello, World!";
  char read_buffer[5]; // we want to test 5 bytes read
  lit_io = iolit_init(&lit_dev, buf, sizeof(buf));
  assert(lit_io != NULL);
  // read test:
  result = ioread_full(lit_io, &read_buffer, sizeof(read_buffer));
  assert(result >= 0);
  char ans[5];
  strncpy(ans, buf, 5); //print the 5 bytes read
  for (int i = 0; i < 5; i++)
  {
    console_putchar(read_buffer[i]);
    console_putchar(' ');
    console_putchar(ans[i]);
    console_putchar('\n');
    assert(read_buffer[i] == ans[i]);
  }
  size_t buf_sz;
  // Get length
  result = ioctl(lit_io, IOCTL_GETLEN, &buf_sz);
  assert(buf_sz == sizeof(buf));
  size_t pos;
  // Get position
  result = ioctl(lit_io, IOCTL_GETPOS, &pos);
  assert(pos == 5);
  // Set position
  result = ioseek(lit_io, 0);
  result = ioctl(lit_io, IOCTL_GETPOS, &pos);
  assert(pos == 0);
  char write_buffer[] = "DENIED";
  // write test:
  result = iowrite(lit_io, &write_buffer, sizeof(write_buffer));
  char buf3[sizeof(buf)];
  result = ioseek(lit_io, 0);
  assert(result >= 0);
  result = ioread_full(lit_io, &buf3, sizeof(buf));
  for (int i = 0; i < sizeof(buf); i++)
  {
    console_putchar(buf3[i]);
  }
  kprintf("\n");
  assert(strcmp(buf3, buf) == 0);

  /*
   * Here ends the test for io_lit device FULL I/O
   */

  /*
   * Here starts the test for Full I/O for the vioblk device
   */
  struct io_intf *blkio = NULL;
  result = device_open(&blkio, "blk", 0); // calls vioblk_open
  assert(result >= 0);
  boot_block_t boot_block;
  // read the boot block
  result = ioread(blkio, &boot_block, BLOCK_SIZE);
  assert(result >= 0);
  // check if the boot block has been read correctly
  assert(boot_block.num_dentry == 3); // number of dentries for the test image
  assert(boot_block.num_inodes == 3); // number of inodes for the test image
  assert(boot_block.num_data == 14); // number of data blocks for the test image
  boot_block_t boot_block2 = boot_block;
  boot_block2.num_dentry = 4; // number of inodes for the test image
  ioseek(blkio, 0);
  result = iowrite(blkio, &boot_block2, BLOCK_SIZE);
  assert(result >= 0);
  ioseek(blkio, 0);
  boot_block_t boot_block3;
  // read the boot block
  result = ioread(blkio, &boot_block3, BLOCK_SIZE);
  assert(result >= 0);
  // check if the boot block has been updated
  assert(boot_block3.num_dentry == 4); // number of dentry for the test image
  assert(boot_block3.num_inodes == 3); // number of inodes for the test image
  assert(boot_block3.num_data == 14); // number of data blocks for the test image
  // restore the original boot block
  ioseek(blkio, 0);
  result = iowrite(blkio, &boot_block, BLOCK_SIZE);
  assert(result >= 0);
  ioseek(blkio, 0);
  int length;
  size_t target_pos, curr_pos, blksz;
  target_pos = 1; // random pos to test
  blkio->ops->ctl(blkio, IOCTL_GETLEN, &length);
  blkio->ops->ctl(blkio, IOCTL_GETBLKSZ, &blksz);
  blkio->ops->ctl(blkio, IOCTL_SETPOS, &target_pos);
  blkio->ops->ctl(blkio, IOCTL_GETPOS, &curr_pos);
  assert(curr_pos == target_pos);
  kprintf("%d", length);
  assert(length == 73728); // length for the test image
  assert(blksz == 512); // default block size

  ioseek(blkio,0);
  /*
   * Here ends the test for Full I/O for the vioblk device
   */

  /*
   * Here starts the test for Full I/O for the file system driver
   */

  result = fs_mount(blkio);
  assert(result >= 0);
  struct io_intf *fs_io1;
  result = fs_open("helloworld.txt", &fs_io1);
  assert(result >= 0);
  struct io_intf *fs_io2;
  result = fs_open("helloworld.txt", &fs_io2);
  assert(result >= 0);
  assert(fs_io1 != fs_io2);
  // Get size
  size_t size;
  result = ioctl(fs_io1, IOCTL_GETLEN, &size);
  assert(size == 435); // the size of the file helloworld.txt in our test image
  char read_gold[] = "[Chorus]";
  char read_buf[8]; // we want to test 8 bytes read "[Chorus]"
  result = ioread_full(fs_io1, read_buf, 8);
  assert(result >= 0);

  for (int i = 0; i < 8; i++)
  {
    console_putchar(read_buf[i]);
    console_putchar(' ');
    console_putchar(read_gold[i]);
    console_putchar('\n');
    assert(read_buf[i] == read_gold[i]);
  }

  result = ioctl(fs_io1, IOCTL_GETPOS, &pos);
  assert(pos == 8);
  result = ioctl(fs_io2, IOCTL_GETPOS, &pos);
  assert(pos == 0);
  result = ioseek(fs_io2, 10); // set pose to 10
  assert(result >= 0);
  result = ioctl(fs_io2, IOCTL_GETPOS, &pos);
  assert(pos == 10); // the pose get should be 10 as well
  char write_buf[] = "reveal the ultimate secrect";
  result = iowrite(fs_io2, write_buf, sizeof(write_buf));
  assert(result >= 0);
  char read_buf2[sizeof(write_buf)];
  result = ioseek(fs_io1, 10); // set pose to 10 again, the read result should be the same we wrote
  assert(result >= 0);
  result = ioread_full(fs_io1, read_buf2, sizeof(write_buf));
  assert(result >= 0);
  assert(strcmp(read_buf2, write_buf) == 0);
  size_t blk_sz;
  result = ioctl(fs_io1, IOCTL_GETBLKSZ, &blk_sz);
  assert(blk_sz == 4096); // the default file block size
  fs_close(fs_io1);
  fs_close(fs_io2);

  /*
   * Here ends the test for Full I/O for the file system driver
   */
  /*
   * Here starts the test for Full I/O for the ELF loader
   */
  struct io_intf *elf_io;
  result = fs_open("enum.txt", &elf_io);
  assert(result >= 0);
  void (*entry)(struct io_intf *io) = NULL;
  result = elf_load(elf_io, &entry);
  assert(result == -EBADFMT);
  kprintf("bad format for enum.txt\n");
  fs_close(elf_io);
  result = fs_open("helloworld.txt", &elf_io);
  assert(result >= 0);
  result = elf_load(elf_io, &entry);
  // kprintf("result: %d\n", result);
  kprintf("bad format for helloworld.txt\n");
  assert(result == -EBADFMT);
  fs_close(elf_io);
  result = fs_open("trek", &elf_io);
  assert(result >= 0);
  result = elf_load(elf_io, &entry);
  assert(result >= 0);
  assert(*entry == (void *)0x8010527c); // the entry address of trek after link
  /*
   * Here ends the test for Full I/O for the ELF loader
   */
  kprintf("All tests passed!\n");
}