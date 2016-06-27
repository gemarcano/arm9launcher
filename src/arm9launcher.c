#include <ctr9/io.h>
#include <ctr9/io/ctr_fatfs.h>
#include <ctr9/ctr_system.h>
#include <ctr/hid.h>

#include <stdlib.h>

int main(int argc, char *argv[])
{
	if (argc == 2)
	{
		ctr_nand_interface nand_io;
		ctr_nand_crypto_interface ctr_io;
		ctr_nand_crypto_interface twl_io;
		ctr_sd_interface sd_io;
		ctr_fatfs_initialize(&nand_io, &ctr_io, &twl_io, &sd_io);

		FATFS fs;
		FIL fil;

		f_mount(&fs, "SD:", 0);
		if (FR_OK != f_open(&fil, argv[0], FA_READ | FA_OPEN_EXISTING))
		{
			return 1;
		}

		size_t offset = (size_t)strtol(argv[1], NULL, 0);

		f_lseek(&fil, offset);
		UINT br;
		f_read(&fil, (void*)0x23F00000, f_size(&fil) - offset, &br);
		ctr_flush_cache();
		((void (*)(void))0x23F00000)();
	}
	return 0;
}

