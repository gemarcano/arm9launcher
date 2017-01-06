/*******************************************************************************
 * Copyright (C) 2016 Gabriel Marcano
 *
 * Refer to the COPYING.txt file at the top of the project directory. If that is
 * missing, this file is licensed under the GPL version 2.0 or later.
 *
 ******************************************************************************/

#include <elf.h>

#include <ctrelf.h>

#include <ctr9/io.h>
#include <ctr9/ctr_system.h>
#include <ctr9/io/ctr_drives.h>
#include <ctr9/ctr_cache.h>
#include <ctr9/sha.h>

#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>

#define PAYLOAD_ADDRESS (0x23F00000)
#define PAYLOAD_POINTER ((void*)PAYLOAD_ADDRESS)
#define PAYLOAD_FUNCTION ((void (*)(void))PAYLOAD_ADDRESS)

inline static void vol_memcpy(volatile void *dest, volatile void *sorc, size_t size)
{
	volatile uint8_t *dst = dest;
	volatile uint8_t *src = sorc;
	while(size--)
		dst[size] = src[size];
}

static int set_position(FILE *file, uint64_t position)
{
	if (fseek(file, 0, SEEK_SET)) return -1;
	while (position > LONG_MAX)
	{
		long pos = LONG_MAX;
		if (fseek(file, pos, SEEK_CUR)) return -1;
		position -= LONG_MAX;
	}

	if (fseek(file, position, SEEK_CUR)) return -1;
	return 0;
}

void ctr_libctr9_init(void);

int main(int argc, char *argv[])
{
	ctr_libctr9_init();
	if (argc == 3)
	{
		//Initialize all possible default IO systems
		FILE *fil = fopen(argv[0], "rb");
		if (!fil)
		{
			return -1;
		}

		Elf32_Ehdr header;
		load_header(&header, fil);
		fseek(fil, 0, SEEK_SET);

		//Restore otp hash
		vol_memcpy(REG_SHAHASH, argv[2], 32);

		if (check_elf(&header)) //ELF
		{
			load_segments(&header, fil);
			((void (*)(int, const char *[]))(header.e_entry))(0, NULL);
		}
		else
		{
			//Read payload, then jump to it
			size_t offset = (size_t)strtol(argv[1], NULL, 0);
			struct stat st;
			fstat(fileno(fil), &st);
			size_t payload_size = ((size_t)st.st_size) - offset; //FIXME Should we limit the size???

			set_position(fil, offset);

			fread(PAYLOAD_POINTER, payload_size, 1, fil);
			fclose(fil);

			ctr_cache_clean_data_range(PAYLOAD_POINTER, (void*)(PAYLOAD_ADDRESS + payload_size));
			ctr_cache_flush_instruction_range(PAYLOAD_POINTER, (void*)(PAYLOAD_ADDRESS + payload_size));
			ctr_cache_drain_write_buffer();

			((void (*)(int, const char *[]))PAYLOAD_ADDRESS)(0, NULL);
		}
	}
	return 0;
}

