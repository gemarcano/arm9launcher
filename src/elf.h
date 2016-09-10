#ifndef ELF_H_
#define ELF_H_

#include <ctr9/io.h>
#include <ctrelf.h>

void load_header(Elf32_Ehdr *header, FIL *file);
int load_segment(const Elf32_Phdr *header, FIL *file);
int load_segments(const Elf32_Ehdr *header, FIL *file);
bool check_elf(Elf32_Ehdr *header);
#endif//ELF_H_
