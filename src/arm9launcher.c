/*******************************************************************************
 * Copyright (C) 2016 Gabriel Marcano
 *
 * Refer to the COPYING.txt file at the top of the project directory. If that is
 * missing, this file is licensed under the GPL version 2.0 or later.
 *
 ******************************************************************************/

#include <ctr9/io.h>
#include <ctr9/io/ctr_fatfs.h>
#include <ctr9/ctr_system.h>
#include <ctr/hid.h>
#include <ctr9/ctr_cache.h>

#include <stdlib.h>

#define PAYLOAD_ADDRESS (0x23F00000)
#define PAYLOAD_POINTER ((void*)PAYLOAD_ADDRESS)
#define PAYLOAD_FUNCTION ((void (*)(void))PAYLOAD_ADDRESS)

int main(int argc, char *argv[])
{
	if (argc == 2)
	{
		//Initialize all possible default IO systems
		ctr_nand_interface nand_io;
		ctr_nand_crypto_interface ctr_io;
		ctr_nand_crypto_interface twl_io;
		ctr_sd_interface sd_io;
		ctr_fatfs_initialize(&nand_io, &ctr_io, &twl_io, &sd_io);

		FATFS fs[4];
		FIL fil;

		f_mount(&fs[0], "SD:", 0);
		f_mount(&fs[1], "CTRNAND:", 0);
		f_mount(&fs[2], "TWLN:", 0);
		f_mount(&fs[3], "TWLN:", 0);

		if (FR_OK != f_open(&fil, argv[0], FA_READ | FA_OPEN_EXISTING))
		{
			return 1;
		}

		//Read payload, then jump to it
		size_t offset = (size_t)strtol(argv[1], NULL, 0);
		size_t payload_size = f_size(&fil) - offset; //FIXME Should we limit the size???

		f_lseek(&fil, offset);

		UINT br;
		f_read(&fil, PAYLOAD_POINTER, payload_size, &br);
		f_close(&fil);

		ctr_cache_clean_data_range(PAYLOAD_POINTER, (void*)(PAYLOAD_ADDRESS + payload_size));
		ctr_cache_flush_instruction_range(PAYLOAD_POINTER, (void*)(PAYLOAD_ADDRESS + payload_size));
		ctr_cache_drain_write_buffer();

		((void (*)(void))PAYLOAD_ADDRESS)();
	}
	return 0;
}

