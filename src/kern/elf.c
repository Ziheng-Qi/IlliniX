#include "elf.h"

/**
 * @brief Loads an executable ELF file into memory and returns the entry point.
 *
 * @param io Pointer to the I/O interface, typically a file, from which the image is to be loaded.
 * @param entry Pointer to a function pointer that will be filled in with the address of the entry point of the ELF file.
 * @return int Returns 0 on success or a negative error code on error.
 */
int elf_load(struct io_intf *io, void (**entryptr)(struct io_intf *io))
{
  Elf64_Ehdr ehdr;

  int result;
  int phnum;
  int phentsize;

  console_printf("Loading ELF file\n");
  // Read the ELF header
  console_printf("About to load %d bytes\n", sizeof(Elf64_Ehdr));
  result = io->ops->read(io, &ehdr, sizeof(Elf64_Ehdr));
  if (result < 0)
    return result;

  // Check the ELF magic number
  if (ehdr.e_ident[0] != ELF_MAGIC0 || ehdr.e_ident[1] != ELF_MAGIC1 || ehdr.e_ident[2] != ELF_MAGIC2 || ehdr.e_ident[3] != ELF_MAGIC3)
    return -EBADFMT;

  // Read the program headers
  phnum = ehdr.e_phnum;
  phentsize = ehdr.e_phentsize;

  debug("phnum: %d, phentsize: %d", phnum, phentsize);

  for (int i = 0; i < phnum; i++)
  {
    Elf64_Phdr phdr;
    io->ops->ctl(io, IOCTL_SETPOS, ehdr.e_phoff + i * phentsize);
    result = io->ops->read(io, &phdr, phentsize);
    if (result < 0)
      return result;

    if (phdr.p_type == PT_LOAD)
    {
      if (phdr.p_vaddr < ENTRY_POINT_MIN || phdr.p_vaddr + phdr.p_filesz > ENTRY_POINT_MAX)
        return -EINVAL;
      io->ops->ctl(io, IOCTL_SETPOS, phdr.p_offset);
      result = io->ops->read(io, (void *)phdr.p_vaddr, phdr.p_filesz);
      if (result < 0)
        return result;
    }
  }

  // verify the entry point is set between 0x80100000 and 0x81000000

  // Set the entry point
  *entryptr = (void (*)(struct io_intf *))ehdr.e_entry;

  return 0;
}
