#include <ctr9/io.h>
#include <ctr9/io/ctr_fatfs.h>
#include <ctr/printf.h>
#include <ctr/hid.h>
#include <ctr9/ctr_system.h>
#include <ctr/draw.h>
#include <ctr/console.h>

void on_error(const char *error);

int main()
{
	draw_init((draw_s*)0x23FFFE00);
	console_init(0xFFFFFF, 0);
	draw_clear_screen(SCREEN_TOP, 0x111111);

	ctr_nand_interface nand_io;
	ctr_nand_crypto_interface ctr_io;
	ctr_nand_crypto_interface twl_io;
	ctr_sd_interface sd_io;

	int result = ctr_fatfs_initialize(&nand_io, &ctr_io, &twl_io, &sd_io);

	if (result)
	{
		on_error("Failed to initialize IO system!");
	}

	FATFS fs;
	FIL bootloader;

	f_mount(&fs, "SD:", 0);
	if (FR_OK != f_open(&bootloader, "SD:/arm9launcher.bin", FA_READ | FA_OPEN_EXISTING))
	{
		on_error("Failed to open bootloader file!");
	}

	UINT br;
	f_read(&bootloader, (void*)0x20000000, f_size(&bootloader), &br);
	
	printf("Jumping to payload...\n");
	ctr_flush_cache();
	char payload[256] = "SD:/arm9loaderhax.bin";
	char *args[] = { payload };
	((int(*)(int, char*[]))0x20000000)(1, args);

	input_wait();
	ctr_system_poweroff();
	return 0;
}

void on_error(const char *error)
{
	printf(error);
	printf("\nPress any key to shutdown.");
	input_wait();
	ctr_system_poweroff();
}


