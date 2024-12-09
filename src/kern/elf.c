// #define _ELF_H_
#include "elf.h"
// #define ELF_TEST_USER
#define ELF_TEST_FLAG 0
uint_fast8_t phdr_flag_to_pte_flag(uint_fast32_t phdr_flags)
{
    uint_fast8_t pte_flags = 0;
    if (phdr_flags & PF_R)
        pte_flags |= PTE_R;
    if (phdr_flags & PF_W)
        pte_flags |= PTE_W;
    if (phdr_flags & PF_X)
        pte_flags |= PTE_X;
    return pte_flags;
}

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
            if (pte == NULL)
                return -EACCESS;
            // check if the vaddr is already mapped
            if ((pte->flags) & PTE_V)
                return -EACCESS;
            #ifndef ELF_TEST_USER
            uint_fast8_t pte_flags = phdr_flag_to_pte_flag(prog_hdr.p_flags) | PTE_U;
            #else
            uint_fast8_t pte_flags = phdr_flag_to_pte_flag(prog_hdr.p_flags);
            #endif
            uintptr_t vaddr = prog_hdr.p_vaddr;
            kprintf("prog_hdr.addr: %x\n", vaddr);
            vaddr = (uintptr_t)(memory_alloc_and_map_range(vaddr, prog_hdr.p_filesz, PTE_R | PTE_W | PTE_U));
            kprintf("loaded vaddr: %x\n", vaddr);
            result = ioread(io, (void *)vaddr, prog_hdr.p_filesz);
            switch (ELF_TEST_FLAG){
                case PTE_R:
                    memory_set_range_flags((void *)vaddr, prog_hdr.p_filesz, pte_flags & !(PTE_R));
                    break;
                case PTE_X:
                    memory_set_range_flags((void *)vaddr, prog_hdr.p_filesz, pte_flags & !(PTE_X));
                    break;
                case PTE_W:
                    memory_set_range_flags((void *)vaddr, prog_hdr.p_filesz, pte_flags & !(PTE_W));
                    break;
                default:
                    memory_set_range_flags((void *)vaddr, prog_hdr.p_filesz, pte_flags);
                    break;
            }
            if (result < 0)
                return result;
        }
    }
    // set entry point
    *entryptr = (void(*)(void)) elf_hdr.e_entry;
    return 0;
}
