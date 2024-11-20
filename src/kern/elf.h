//           elf.h - ELF executable loader
//          

#ifndef _ELF_H_
#define _ELF_H_

#include "io.h"
#include "fs.h"
#include "config.h"

#define Elf64_Addr uint64_t
#define Elf64_Off  uint64_t


#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    Elf64_Addr     e_entry;
    Elf64_Off      e_phoff;
    Elf64_Off      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t   p_type;
    uint32_t   p_flags;
    Elf64_Off  p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    uint64_t   p_filesz;
    uint64_t   p_memsz;
    uint64_t   p_align;
} Elf64_Phdr;

// e_ident[] index
#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define	EI_DATA		5
#define	EI_VERSION	6
#define	EI_OSABI	7
#define	EI_PAD		8

// EI_MAG
#define	ELFMAG0		0x7f		
#define	ELFMAG1		'E'
#define	ELFMAG2		'L'
#define	ELFMAG3		'F'
#define	ELFMAG		"\177ELF"
#define	SELFMAG		4

// EI_CLASS
#define	ELFCLASSNONE	0		
#define	ELFCLASS32	    1
#define	ELFCLASS64	    2
#define	ELFCLASSNUM	    3

// EI_DATA
#define ELFDATANONE	    0		
#define ELFDATA2LSB	    1
#define ELFDATA2MSB	    2

// EI_VERSION
#define EV_NONE		    0		
#define EV_CURRENT	    1
#define EV_NUM		    2

// OS ABI
#define ELFOSABI_NONE   0  

// elf_header -> e.machine
#define EM_ARM  40

// program_header -> p_type
#define PT_LOAD 1

// valid load address
#define VALID_ADDR_LOW   0x80100000  
#define VALID_ADDR_HIGH  0x81000000  

// fs_ioctl cmd
#define IOCTL_GETPOS        3
#define IOCTL_SETPOS        4

//           arg1: io interface from which to load the elf arg2: pointer to void
//           (*entry)(struct io_intf *io), which is a function pointer elf_load fills in
//           w/ the address of the entry point

//           int elf_load(struct io_intf *io, void (**entry)(struct io_intf *io)) Loads an
//           executable ELF file into memory and returns the entry point. The /io/
//           argument is the I/O interface, typically a file, from which the image is to
//           be loaded. The /entryptr/ argument is a pointer to an function pointer that
//           will be filled in with the entry point of the ELF file.
//           Return 0 on success or a negative error code on error.

int elf_load(struct io_intf *io, void (**entryptr)(void));

//           _ELF_H_
#endif