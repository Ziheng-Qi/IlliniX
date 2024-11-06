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

int main() {
  // struct io_lit lit;
  // uint8_t buf[sizeof(Elf64_Ehdr)];

  // struct io_intf* io = iolit_init(&lit, &buf, _companion_f_end - _companion_f_start);
  // void (**entryptr)(struct io_intf *io);
  // int result = elf_load(io, entryptr);
  // if (result == 0)
  //     console_printf("Elf_load succeeds");
  // else if (result < 0)
  //     console_printf("Elf_load fails");
  // return 0;
  struct io_lit lit;
  struct io_intf* trek_io = iolit_init(&lit, _companion_f_start, _companion_f_end - _companion_f_start);
  // int result = fs_open("trek", &trek_io); // 打开 trek 文件
  // if (result < 0) {
  //     console_printf("Result: %d\n", result);
  //     console_printf("Error opening trek file\n");
  //     return 1; // 错误处理
  // }
  // if (result == 0) 
  //     console_printf("Successfully opening trek file\n");

  void (*entry)(struct io_intf*);
  int result = elf_load(trek_io, &entry);

  if (result == 0)
    console_printf("Success to load trek file: %d\n", result);
  if (result != 0) {
      console_printf("Failed to load trek file: %d\n", result);
      return 1; // 错误处理
  }

    // 验证入口指针
  if (*entry != NULL) {
      console_printf("Trek file loaded successfully, entry pointer: %p\n", (void*)entry);
        // 可以调用 entry 来执行程序
  } else {
      console_printf("Entry pointer is NULL\n");
  }

  return 0;
}


