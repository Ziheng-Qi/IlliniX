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


// Rubric test for elf_loader. We use io_lit to do unit test, avoid using filesystem
// and virtio. We check:
// 1. rejects an iointf not providing a little-endian riscv64 executable
// 2. loads required sections into kernel memory
// 3. properly updates entry pointer reading from an io_intf

int main() {
  // Here we test 2 & 3 by performing an elfload to "trek". 
  struct io_lit lit;
  struct io_intf* trek_io = iolit_init(&lit, _companion_f_start, _companion_f_end - _companion_f_start);

  void (*entry)(struct io_intf*);
  int result = elf_load(trek_io, &entry);

  assert(result == 0);
  if (result == 0)
    console_printf("Success to load trek file: %d\n", result);
  if (result != 0) {
      console_printf("Failed to load trek file: %d\n", result);
      return 1; 
  }
  assert(*entry != NULL);
  if (*entry != NULL) {
      console_printf("Trek file loaded successfully, entry pointer: %p\n", (void*)entry);
    
  } else {
      console_printf("Entry pointer is NULL\n");
  }

  // Here we test 1 by imitating a Elf_header but with big-endian to see if elf_load
  // rejects it.
  // We only have a elf_header, no program header because we are simply testing the 
  // little-edian byte. 
  uint8_t buffer[sizeof(Elf64_Ehdr)] = {0};
    buffer[0] = 0x7f; // EI_MAG0
    buffer[1] = 'E';  // EI_MAG1
    buffer[2] = 'L';  // EI_MAG2
    buffer[3] = 'F';  // EI_MAG3
    buffer[4] = ELFCLASS64; // EI_CLASS 
    buffer[5] = ELFDATA2MSB; // EI_DATA 
    buffer[6] = EV_CURRENT; // EI_VERSION

    struct io_lit lit2;
    struct io_intf* io2 = iolit_init(&lit2, buffer, sizeof(buffer));
    void (*entry2)(struct io_intf*);

    result = elf_load(io2, &entry2);
    assert(result < 0);
    if (result < 0) {
        console_printf("Test passed: Non-little-endian ELF was rejected.\n");
    } else {
        console_printf("Test failed: Unexpected result %d\n", result);
    }

  return 0;
}


