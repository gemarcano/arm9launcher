#ifndef ELF_H_
#define ELF_H_

#include <ctr9/io.h>
#include <ctrelf.h>
#include <stdio.h>

void load_header(Elf32_Ehdr *header, FILE *file);
int load_segment(const Elf32_Phdr *header, FILE *file);
int load_segments(const Elf32_Ehdr *header, FILE *file);
bool check_elf(Elf32_Ehdr *header);
#endif//ELF_H_
