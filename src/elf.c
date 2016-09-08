#include <ctr9/io.h>
#include <elf.h>

#include <string.h>

void load_header(Elf32_Ehdr *header, FIL *file)
{
	f_lseek(file, 0);
	char buffer[sizeof(*header)];
	UINT br;
	int res = f_read(file, buffer, sizeof(buffer), &br);

	elf_load_header(header, buffer);
}

void load_segment(const Elf32_Phdr *header, FIL *file)
{
	size_t program_size = header->p_filesz;
	size_t mem_size = header->p_memsz;
	void *location = (void*)(header->p_vaddr);
	
	f_lseek(file, header->p_offset);
	UINT br;
	f_read(file, location, program_size, &br);
	memset(program_size + (char*)location, 0, mem_size - program_size);
}

void load_segments(const Elf32_Ehdr *header, FIL *file)
{
	size_t pnum = header->e_phnum;
	char buffer[pnum][header->e_phentsize];
	UINT br;

	f_lseek(file, header->e_phoff);
	int res = f_read(file, buffer, sizeof(buffer), &br);

	for (size_t i = 0; i < pnum; ++i)
	{
		Elf32_Phdr pheader;
		elf_load_program_header(&pheader, buffer[i]);
		load_segment(&pheader, file);
	}
}

bool check_elf(Elf32_Ehdr *header)
{
	return header->e_ident[EI_MAG0] == (char)0x7f &&
		header->e_ident[EI_MAG1] == 'E' &&
		header->e_ident[EI_MAG2] == 'L' &&
		header->e_ident[EI_MAG3] == 'F';
}

