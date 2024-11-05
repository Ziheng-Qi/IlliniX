//            elf.h - ELF executable loader
//           

#ifndef _ELF_H_
#define _ELF_H_

#include "io.h"
#include "fs.h"
#include "error.h"

//            arg1: io interface from which to load the elf arg2: pointer to void
//            (*entry)(struct io_intf *io), which is a function pointer elf_load fills in
//            w/ the address of the entry point

//            int elf_load(struct io_intf *io, void (**entry)(struct io_intf *io)) Loads an
//            executable ELF file into memory and returns the entry point. The /io/
//            argument is the I/O interface, typically a file, from which the image is to
//            be loaded. The /entryptr/ argument is a pointer to an function pointer that
//            will be filled in with the entry point of the ELF file.
//            Return 0 on success or a negative error code on error.
#define EI_NIDENT 16

#define Elf64_Addr uint64_t
#define Elf64_Off uint64_t
#define Elf64_Section uint16_t
#define Elf64_Versym uint16_t
#define ElfByte unsigned char
#define Elf64_Half uint16_t
#define Elf64_Sword int32_t
#define Elf64_Word uint32_t
#define Elf64_Sxword int64_t
#define Elf64_Xword uint64_t

typedef struct
{
  unsigned char e_ident[EI_NIDENT];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;
  Elf64_Off e_shoff;
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;
} Elf64_Ehdr;

typedef struct
{
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;

typedef struct
{
  Elf64_Word sh_name;
  Elf64_Word sh_type;
  Elf64_Xword sh_flags;
  Elf64_Addr sh_addr;
  Elf64_Off sh_offset;
  Elf64_Xword sh_size;
  Elf64_Word sh_link;
  Elf64_Word sh_info;
  Elf64_Xword sh_addralign;
  Elf64_Xword sh_entsize;
} Elf64_Shdr;

typedef struct
{
  Elf64_Word st_name;
  ElfByte st_info;
  ElfByte st_other;
  Elf64_Half st_shndx;
  Elf64_Addr st_value;
  Elf64_Xword st_size;
} Elf64_Sym;

typedef struct
{
  Elf64_Half si_boundto;
  Elf64_Half si_flags;
} Elf64_Verdef;

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_TLS 7
#define PT_NUM 8

#define ELF_MAGIC0 0x7f
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'

#define ENTRY_POINT_MIN 0x80100000
#define ENTRY_POINT_MAX 0x81000000

#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8

#define PF_R 0x4
#define PF_W 0x2
#define PF_X 0x1

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

int elf_load(struct io_intf *io, void (**entryptr)(struct io_intf *io));

//            _ELF_H_

#endif
