/*******************************************************************************
 * Copyright (C) 2016 Gabriel Marcano
 *
 * Refer to the COPYING.txt file at the top of the project directory. If that is
 * missing, this file is licensed under the GPL version 2.0 or later.
 *
 ******************************************************************************/

#include "a9l_config.h"

#include <ctr9/io.h>
#include <ctr9/io/ctr_fatfs.h>
#include <ctr9/ctr_system.h>
#include <ctr9/ctr_hid.h>
#include <ctr9/ctr_cache.h>

#include <ctr/hid.h>
#include <ctr/draw.h>
#include <ctr/console.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ctr/printf.h>

#define A9L_ADDR 0x08000000u

static void on_error(const char *error);
static const a9l_config_entry* select_payload(const a9l_config *config, ctr_hid_button_type buttons);
static const char * find_file(const char *path);

static void initialize_io(ctr_nand_interface *nand_io, ctr_nand_crypto_interface *ctr_io, ctr_sd_interface *sd_io);
static void initialize_fatfs(FATFS fs[]);
static void load_bootloader(void);
static void handle_payload(char *path, size_t path_size, char *offset, size_t offset_size, ctr_hid_button_type buttons_pressed);

int main()
{
	//Before anything else, immediately record the buttons to use for boot
	ctr_hid_button_type buttons_pressed = ctr_hid_get_buttons();

	//Prepare screen for diagnostic usage
	draw_init((draw_s*)0x23FFFE00);
	console_init(0xFFFFFF, 0);

	//IO initialization
	ctr_nand_interface nand_io;
	ctr_nand_crypto_interface ctr_io;
	ctr_sd_interface sd_io;
	initialize_io(&nand_io, &ctr_io, &sd_io);

	//Initialize fatfs
	FATFS fs[2];
	initialize_fatfs(fs);

	load_bootloader();

	char payload[256] = { 0 };
	char offset[256] = { 0 };
	handle_payload(payload, sizeof(payload), offset, sizeof(offset), buttons_pressed);

	printf("Jumping to bootloader...\n");
	char *args[] = { payload, offset };

	//Bootloader has been cleaned to memory, and whatever is in the stack is safe
	//since the bootloader doesn't flush the cache without cleaning. Just for
	//safety, drain the write buffer before jumping.
	ctr_cache_drain_write_buffer();

	//Jump to bootloader
	int bootloader_result = ((int(*)(int, char*[]))A9L_ADDR)(2, args);

	//Re-init screen structures in case bootloader altered the memory controlling
	//it.
	console_init(0xFFFFFF, 0);
	if (bootloader_result)
	{
		printf("An error was reported by the bootloader!\nError return: %d\n", bootloader_result);
	}
	printf("Returned from the bootloader. Press any key to power down\n");
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
	for (size_t i = 0; i < num_of_entries; ++i)
	{
		a9l_config_entry *entry = a9l_config_get_entry(config, i);
		if (entry->buttons == buttons)
		{
			return entry;
		}
	}
	return NULL;
}

static const char * find_file(const char *path)
{
	f_chdrive("SD:");
	if (ctr_sd_interface_inserted() && (FR_OK == f_stat(path, NULL)))
	{
		return "SD:";
	}
	else
	{
		f_chdrive("CTRNAND:");
		if (FR_OK == f_stat(path, NULL))
		{
			return "CTRNAND:";
		}
	}

	return NULL;
}

static void initialize_io(ctr_nand_interface *nand_io, ctr_nand_crypto_interface *ctr_io, ctr_sd_interface *sd_io)
{
	int result = ctr_fatfs_internal_initialize(nand_io, ctr_io, NULL);

	if (result)
	{
		on_error("Failed to initialize internal IO system!");
	}

	result = ctr_fatfs_sd_initialize(sd_io);

	if (result && ctr_sd_interface_inserted())
	{
		on_error("SD card detected but failed to initialize it!");
	}
}

static void initialize_fatfs(FATFS fs[])
{
	if ((ctr_sd_interface_inserted() && (FR_OK != f_mount(&fs[0], "SD:", 1))) ||
		(FR_OK != f_mount(&fs[1], "CTRNAND:", 1)))
	{
		on_error("Failed to mount filesystems!");
	}
}

static void load_bootloader(void)
{
	const char * drive = find_file("/arm9launcher.bin");
	if (!drive)
	{
		on_error("Unable to find bootloader file!");
	}

	FIL bootloader;
	f_chdrive(drive);
	if (FR_OK != f_open(&bootloader, "/arm9launcher.bin", FA_READ | FA_OPEN_EXISTING))
	{
		on_error("Failed to open bootloader file!");
	}

	UINT br;
	size_t bootloader_size = f_size(&bootloader); //FIXME we should limit the size...
	f_read(&bootloader, (void*)A9L_ADDR, bootloader_size, &br);
	f_close(&bootloader);

	//Make sure the bootloader makes it to memory
	ctr_cache_clean_data_range((void*)A9L_ADDR, (void*)(A9L_ADDR + bootloader_size));
	ctr_cache_flush_instruction_range((void*)A9L_ADDR, (void*)(A9L_ADDR + bootloader_size));
}

static void handle_payload(char *path, size_t path_size, char *offset, size_t offset_size, ctr_hid_button_type buttons_pressed)
{
	FIL config_file;
	UINT br;
	const char* drive = find_file("/arm9launcher.cfg");
	if (!drive)
	{
		on_error("Unable to find configuration file!");
	}

	f_chdrive(drive);
	if (FR_OK != f_open(&config_file, "/arm9launcher.cfg", FA_READ | FA_OPEN_EXISTING))
	{
		on_error("Failed to open bootloader config!");
	}

	size_t buffer_size = f_size(&config_file) + 1; //FIXME we should limit the size...
	char *buffer = malloc(buffer_size);
	f_read(&config_file, buffer, buffer_size - 1, &br);
	f_close(&config_file);

	buffer[buffer_size-1] = '\0'; //Make sure buffer, which should be all text, is null terminated.

	//Parse configuration and make sense of it
	a9l_config config = { 0 };

	if (!a9l_config_read_json(&config, buffer))
	{
		free(buffer);
		on_error("Failed to parse JSON configuration file");
	}
	free(buffer);

	//Using a fixed buffer because this will be passed to bootloader via the stack.
	char payload[256] = { 0 };
	const a9l_config_entry *entry = select_payload(&config, buttons_pressed);
	if (entry)
	{
		size_t payload_len = strlen(entry->payload) + 1;
		if (payload_len > path_size)
		{
			on_error("Payload text string in configuration is too long!");
		}
		strncpy(path, entry->payload, payload_len);
	}
	else
	{
		on_error("Failed to identify payload to launch");
	}

	//Using a fixed buffer because this will be passed to bootloader via the stack.
	int supposed_conversion = snprintf(offset, offset_size, "%zu", entry->offset);
	if (supposed_conversion < 0)
	{
		on_error("Failed to convert offset number for payload specified!");
	}

	if ((size_t)supposed_conversion > offset_size)
	{
		on_error("Offset is too long/large!");
	}

	//Done with configuration, ready to jump
	a9l_config_destroy(&config);
}

