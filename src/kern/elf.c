// #define _ELF_H_
#include "elf.h"

/**
 * @brief Loads an executable ELF file into memory and returns the entry point.
 * This funcion takes an io interface (typically a file). It first does validation.
 * It includes checking elf_header.edient[] contents. After we verify the file is valid
 * elf file, we load the image to virtual address (p_vaddr) and set the entrypoint to
 * elf_header.e_entry.
 * @param io IO interface pointer
 * @param entryptr a function pointer elf_load fills in with the address of the entry point
 * @return int 0 if elf_load success, negative value if error 
 */

int elf_load(struct io_intf *io, void (**entryptr)(void)) {
    // TODO: Add virtual memory mappings
    Elf64_Ehdr elf_hdr;
    int result = ioread(io, &elf_hdr, sizeof(elf_hdr));
    // read error
    if (result < 0)
        return result;
    // check if it is valid elf file
    if (elf_hdr.e_ident[EI_MAG0] != ELFMAG0 || elf_hdr.e_ident[EI_MAG1] != ELFMAG1 ||
        elf_hdr.e_ident[EI_MAG2] != ELFMAG2 || elf_hdr.e_ident[EI_MAG3] != ELFMAG3)
        return -EBADFMT;
    if (elf_hdr.e_ident[EI_CLASS] != ELFCLASS64)
        return -EBADFMT;
    if (elf_hdr.e_ident[EI_DATA] != ELFDATA2LSB)
        return -EBADFMT;
    if (elf_hdr.e_ident[EI_VERSION] != EV_CURRENT)
        return -EBADFMT;

    // iterate through program headers, load image if valid
    for (int i = 0; i < elf_hdr.e_phnum; i++) {
        Elf64_Phdr prog_hdr;
        uint64_t pos = elf_hdr.e_phoff + i * elf_hdr.e_phentsize;
        result = ioseek(io, pos);
        if (result < 0)
            return result;
        result = ioread(io, &prog_hdr, elf_hdr.e_phentsize);
        if (result < 0)
            return result;
        // check if type and section addr are both valid

        // cp2: check if the vaddr already mapped, if not, alloc page to it.
        // also check if the filesize is a multiple of page size, so know how many pages to assign 
        if (prog_hdr.p_type == PT_LOAD) {
            if (prog_hdr.p_vaddr < USER_START_VMA || prog_hdr.p_vaddr + prog_hdr.p_filesz > USER_END_VMA)
                return -EINVAL;
            ioseek(io, prog_hdr.p_offset);
            struct pte *active_space_r = active_space_root();
            struct pte *pte = walk_pt(active_space_r, prog_hdr.p_vaddr, 1);

            result = ioread(io, (void*) prog_hdr.p_vaddr, prog_hdr.p_filesz);
            if (result < 0)
                return result;
        }
    }
    // set entry point
    *entryptr = (void(*)(void)) elf_hdr.e_entry;
    // console_printf("Entryptr: %x\n", *entryptr);
    return 0;
}
