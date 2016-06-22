#include <ctr9/io.h>
#include <ctr9/io/ctr_fatfs.h>
#include <ctr9/ctr_system.h>
#include <ctr/draw.h>
#include <ctr/printf.h>
#include <ctr/console.h>

int main(int argc, char *argv[])
{
	draw_init((draw_s*)0x23FFFE00);
	console_init(0xFFFFFF, 0);
	draw_clear_screen(SCREEN_TOP, 0x555555);

	printf("argc: %d, argv[0]: %s\n", argc, argv[0]);

	if (argc == 1)
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
			printf("failed to mount or load\n");
			return 1;
		}

		printf("Attempting to load and run\n");
		UINT br;
		f_read(&fil, (void*)0x23F00000, f_size(&fil), &br);
		draw_clear_screen(SCREEN_SUB, 0x000000);
		draw_clear_screen(SCREEN_TOP, 0x000000);
		ctr_flush_cache();
		((void (*)(void))0x23F00000)();
	}
	return 0;
}

