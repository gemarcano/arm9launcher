#include <ctr9/io.h>
#include <ctr9/io/ctr_fatfs.h>
#include <ctr/printf.h>
#include <ctr/hid.h>
#include <ctr9/ctr_system.h>
#include <ctr/draw.h>
#include <ctr/console.h>
#include <ctr9/ctr_hid.h>

#include <string.h>

#include "a9l_config.h"

void on_error(const char *error);

const a9l_config_entry* select_payload(const a9l_config *config, ctr_hid_button_type buttons);

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

	a9l_config config;
	a9l_config_initialize(&config, 3);
	a9l_config_entry *entry = a9l_config_get_entry(&config, 0);
	entry->payload = "SD:/arm9loaderhax/G9.bin";
	entry->offset = 0;
	entry->buttons = CTR_HID_A;

	entry = a9l_config_get_entry(&config, 1);
	entry->payload = "SD:/arm9loaderhax/uncart.bin";
	entry->offset = 0;
	entry->buttons = CTR_HID_B;

	entry = a9l_config_get_entry(&config, 2);
	entry->payload = "SD:/arm9loaderhax/D9.bin";
	entry->offset = 0;
	entry->buttons = CTR_HID_RT | CTR_HID_LT;

	printf("Press a button to choose boot\n");
	input_wait();
	ctr_hid_button_type buttons_pressed = ctr_hid_get_buttons();
	input_wait();


	char payload[256] = { 0 };
	entry = select_payload(&config, buttons_pressed);
	if (entry)
	{
		strncpy(payload, entry->payload, sizeof(payload));
	}
	
	FATFS fs;
	FIL bootloader;

	f_mount(&fs, "SD:", 0);
	f_mount(&fs, "CTRNAND:", 0);
	if (FR_OK != f_open(&bootloader, "SD:/arm9launcher.bin", FA_READ | FA_OPEN_EXISTING))
	{
		on_error("Failed to open bootloader file!");
	}

	UINT br;
	f_read(&bootloader, (void*)0x20000000, f_size(&bootloader), &br);
	
	printf("Jumping to payload...\n");
	ctr_flush_cache();
	
	char *args[] = { payload };
	((int(*)(int, char*[]))0x20000000)(1, args);

	console_init(0xFFFFFF, 0);

	printf("Returned from the payload. Press any key to power down\n");
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

const a9l_config_entry* select_payload(const a9l_config *config, ctr_hid_button_type buttons)
{
	size_t num_of_entries = a9l_config_get_number_of_entries(config);
	for (size_t i = 0; i < a9l_config_get_number_of_entries(config); ++i)
	{
		a9l_config_entry *entry = a9l_config_get_entry(config, i);
		if (entry->buttons == buttons)
		{
			return entry;
		}
	}
	return NULL;
}

