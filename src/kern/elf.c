// #define _ELF_H_
#include "elf.h"



//           arg1: io interface from which to load the elf arg2: pointer to void
//           (*entry)(struct io_intf *io), which is a function pointer elf_load fills in
//           w/ the address of the entry point

//           int elf_load(struct io_intf *io, void (**entry)(struct io_intf *io)) Loads an
//           executable ELF file into memory and returns the entry point. The /io/
//           argument is the I/O interface, typically a file, from which the image is to
//           be loaded. The /entryptr/ argument is a pointer to an function pointer that
//           will be filled in with the entry point of the ELF file.
//           Return 0 on success or a negative error code on error.

int elf_load(struct io_intf *io, void (**entryptr)(struct io_intf *io)) {
    Elf64_Ehdr elf_hdr;
    int result = ioread(io, &elf_hdr, sizeof(elf_hdr));
    // read error
    if (result < 0)
        return result;
    // validation
    if (elf_hdr.e_ident[EI_MAG0] != ELFMAG0 || elf_hdr.e_ident[EI_MAG1] != ELFMAG1 ||
        elf_hdr.e_ident[EI_MAG2] != ELFMAG2 || elf_hdr.e_ident[EI_MAG3] != ELFMAG3)
        return -2;
    if (elf_hdr.e_ident[EI_CLASS] != ELFCLASS64)
        return -3;
    if (elf_hdr.e_ident[EI_DATA] != ELFDATA2LSB)
        return -4;
    if (elf_hdr.e_ident[EI_VERSION] != EV_CURRENT)
        return -5;


    for (int i = 0; i < elf_hdr.e_phnum; i++) {
        Elf64_Phdr prog_hdr;
        uint64_t pos = elf_hdr.e_phoff + i * elf_hdr.e_phentsize;
        result = ioseek(io, pos);
        if (result < 0)
            return result;
        result = ioread(io, &prog_hdr, elf_hdr.e_phentsize);
        if (result < 0)
            return result;
        // check type and all sections are valid
        if (prog_hdr.p_type == PT_LOAD) {
            if (prog_hdr.p_vaddr < VALID_ADDR_LOW || prog_hdr.p_vaddr + prog_hdr.p_filesz > VALID_ADDR_HIGH)
                return -6;
        }
        ioseek(io, prog_hdr.p_offset);
        console_printf("Loaded segment to address: %p, size: %zu\n", (void*)prog_hdr.p_vaddr, prog_hdr.p_filesz);
        result = ioread(io, (void*) prog_hdr.p_vaddr, prog_hdr.p_filesz);
        if (result < 0)
            return result;
        console_printf("Hello2\n");
    }
    // set entry point
    *entryptr = (void(*) (struct io_intf*)) elf_hdr.e_entry;
    return 0;
}
