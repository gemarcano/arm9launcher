/*******************************************************************************
 * Copyright (C) 2016 Gabriel Marcano
 *
 * Refer to the COPYING.txt file at the top of the project directory. If that is
 * missing, this file is licensed under the GPL version 2.0 or later.
 *
 ******************************************************************************/

#include "a9l_config.h"

#include <ctr9/io.h>
#include <ctr9/ctr_system.h>
#include <ctr9/io/ctr_drives.h>
#include <ctr9/io/ctr_console.h>
#include <ctr9/ctr_hid.h>
#include <ctr9/ctr_cache.h>
#include <ctr9/ctr_system.h>
#include <ctr9/sha.h>

#include <ctr/hid.h>
#include <ctr/draw.h>
#include <ctr/console.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>

static void on_error(const char *error);
static const a9l_config_entry* select_payload(const a9l_config *config, ctr_hid_button_type buttons);
static const char *find_file(const char *path, const char* drives[], size_t number_of_drives);

static void initialize_io(void);
static void handle_payload(char *path, size_t path_size, size_t *offset, ctr_hid_button_type buttons_pressed);

static int boot(const char *path, size_t offset);

inline static void vol_memcpy(volatile void *dest, volatile void *sorc, size_t size)
{
	volatile uint8_t *dst = dest;
	volatile uint8_t *src = sorc;
	while(size--)
		dst[size] = src[size];
}

static uint8_t otp_sha[32];

int main()
{
	//Before anything else, immediately record the buttons to use for boot
	ctr_hid_button_type buttons_pressed = ctr_hid_get_buttons();

	//Now, before libctr9 does anything else, preserve sha state
	vol_memcpy(otp_sha, REG_SHAHASH, 32);


	//Prepare screen for diagnostic usage
	draw_init((draw_s*)0x23FFFE00);
	console_init(0xFFFFFF, 0);

	//IO initialization
	initialize_io();

	char payload[256] = { 0 };
	size_t offset = 0;
	handle_payload(payload, sizeof(payload), &offset, buttons_pressed);

	printf("Jumping to bootloader...\n");

	//Bootloader has been cleaned to memory, and whatever is in the stack is safe
	//since the bootloader doesn't flush the cache without cleaning. Just for
	//safety, drain the write buffer before jumping.
	ctr_cache_drain_write_buffer();

	//Jump to bootloader
	int bootloader_result = boot(payload, offset);

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

static void on_error(const char *error)
{
	printf("%s", error);
	printf("\nPress any key to shutdown.");
	input_wait();
	ctr_system_poweroff();
}

static const a9l_config_entry* select_payload(const a9l_config *config, ctr_hid_button_type buttons)
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

static const char *find_file(const char *path, const char* drives[], size_t number_of_drives)
{
	for (size_t i = 0; i < number_of_drives; ++i)
	{
		ctr_drives_chdrive(drives[i]);
		struct stat st;
		if (stat(path, &st) == 0)
		{
			return drives[i];
		}
	}
	return NULL;
}

static void initialize_io(void)
{
	int result = ctr_console_initialize();

	result |= ctr_drives_initialize();
	result |= ctr_drives_check_ready("CTRNAND:");
	result |= ctr_drives_check_ready("TWLN:");
	result |= ctr_drives_check_ready("TWLP:");

	if (result)
	{
		on_error("Failed to initialize internal IO system!");
	}

	result = ctr_drives_check_ready("SD:");
	if (result && ctr_sd_interface_inserted())
	{
		on_error("SD card detected but failed to initialize it!");
	}
}

static void handle_payload(char *path, size_t path_size, size_t *offset, ctr_hid_button_type buttons_pressed)
{
	FILE *config_file;
	const char *drives[] = { "SD:", "CTRNAND:", "TWLN:", "TWLP:" };
	const char* drive = find_file("/arm9launcher.cfg", drives, 4);
	if (!drive)
	{
		on_error("Unable to find configuration file!");
	}

	ctr_drives_chdrive(drive);
	config_file = fopen("/arm9launcher.cfg", "rb");
	if (config_file == NULL)
	{
		on_error("Failed to open bootloader config!");
	}

	struct stat st = { 0 };
	fstat(fileno(config_file), &st);

	size_t buffer_size = (size_t)(st.st_size) + 1; //FIXME we should limit the size...
	char *buffer = malloc(buffer_size);
	fread(buffer, buffer_size - 1, 1, config_file);
	fclose(config_file);

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

	*offset = entry->offset;

	//Done with configuration, ready to jump
	a9l_config_destroy(&config);
}

#include <elf.h>

#include <ctrelf.h>

#include <ctr9/io.h>
#include <ctr9/ctr_system.h>
#include <ctr/hid.h>
#include <ctr9/ctr_cache.h>
#include <stdlib.h>
#include <limits.h>

#define PAYLOAD_ADDRESS (0x23F00000)
#define PAYLOAD_POINTER ((void*)PAYLOAD_ADDRESS)
#define PAYLOAD_FUNCTION ((void (*)(void))PAYLOAD_ADDRESS)

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

int boot(const char *path, size_t offset)
{
	//Initialize all possible default IO systems
	FILE *fil;

	fil = fopen(path, "rb");
	if (fil == NULL)
	{
		return -1;
	}

	Elf32_Ehdr header;
	load_header(&header, fil);
	fseek(fil, 0, SEEK_SET);

	//restore sha
	vol_memcpy(REG_SHAHASH, otp_sha, 32);

	if (check_elf(&header)) //ELF
	{
		//load_segments handles cache maintenance
		load_segments(&header, fil);
		((void (*)(void))(header.e_entry))();
	}
	else
	{
		struct stat st;
		fstat(fileno(fil), &st);
		size_t payload_size = ((size_t)st.st_size) - offset; //FIXME Should we limit the size???

		set_position(fil, offset);

		fread(PAYLOAD_POINTER, payload_size, 1, fil);
		fclose(fil);

		ctr_cache_clean_data_range(PAYLOAD_POINTER, (void*)(PAYLOAD_ADDRESS + payload_size));
		ctr_cache_flush_instruction_range(PAYLOAD_POINTER, (void*)(PAYLOAD_ADDRESS + payload_size));
		ctr_cache_drain_write_buffer();

		((void (*)(void))PAYLOAD_ADDRESS)();
	}
	return 0;
}

