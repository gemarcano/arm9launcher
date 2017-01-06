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
#include <ctr9/ctr_hid.h>
#include <ctr9/ctr_cache.h>
#include <ctr9/ctr_system.h>
#include <ctr9/io/ctr_drives.h>
#include <ctr9/sha.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>

#define A9L_ADDR 0x20010000u

static void on_error(const char *error);
static const a9l_config_entry* select_payload(const a9l_config *config, ctr_hid_button_type buttons);
static const char *find_file(const char *path, const char* drives[], size_t number_of_drives);

static void initialize_io(void);
static void load_bootloader(void);
static void handle_payload(char *path, size_t path_size, size_t *offset, ctr_hid_button_type buttons_pressed);

static uint8_t otp_sha[32];

inline static void vol_memcpy(volatile void *dest, volatile void *sorc, size_t size)
{
	volatile uint8_t *dst = dest;
	volatile uint8_t *src = sorc;
	while(size--)
		dst[size] = src[size];
}

static void __attribute__((constructor)) save_hash(void)
{
	vol_memcpy(otp_sha, REG_SHAHASH, 32);
}

void ctr_libctr9_init(void);

int main()
{
	//Before anything else, immediately record the buttons to use for boot
	ctr_hid_button_type buttons_pressed = ctr_hid_get_buttons();

	//IO initialization
	initialize_io();

	load_bootloader();

	char payload[256] = { 0 };
	size_t offset;
	handle_payload(payload, sizeof(payload), &offset, buttons_pressed);
	char offset_text[256] = {0};

	snprintf(offset_text, 255, "%zu", offset);

	printf("Jumping to bootloader...\n");
	const char *args[] = { payload, offset_text, (const char*)otp_sha };

	//Bootloader has been cleaned to memory, and whatever is in the stack is safe
	//since the bootloader doesn't flush the cache without cleaning. Just for
	//safety, drain the write buffer before jumping.
	ctr_cache_drain_write_buffer();

	//Jump to bootloader
	int bootloader_result = ((int(*)(int, const char*[]))A9L_ADDR)(3, args);

	//Re-init screen structures in case bootloader altered the memory controlling
	//it.
	ctr_libctr9_init();
	if (bootloader_result)
	{
		printf("An error was reported by the bootloader!\nError return: %d\n", bootloader_result);
	}
	printf("Returned from the bootloader. Press any key to power down\n");
	ctr_input_wait();

	ctr_system_poweroff();
	return 0;
}

void on_error(const char *error)
{
	printf("%s", error);
	printf("\nPress any key to shutdown.");
	ctr_input_wait();
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
	int result = ctr_drives_check_ready("CTRNAND:");
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

static void load_bootloader(void)
{
	const char *drives[] = {"SD:", "CTRNAND:", "TWLN:", "TWLP:" };
	const char * drive = find_file("/arm9launcher.bin", drives, 4);
	if (!drive)
	{
		on_error("Unable to find bootloader file!");
	}

	ctr_drives_chdrive(drive);
	FILE *bootloader = fopen("/arm9launcher.bin", "rb");
	if (bootloader == NULL)
	{
		on_error("Failed to open bootloader file!");
	}

	struct stat st;
	fstat(fileno(bootloader), &st);
	size_t bootloader_size = (size_t)st.st_size;//FIXME we should limit the size...
	fread((void*)A9L_ADDR, bootloader_size, 1, bootloader);
	fclose(bootloader);

	//Make sure the bootloader makes it to memory
	ctr_cache_clean_data_range((void*)A9L_ADDR, (void*)(A9L_ADDR + bootloader_size));
	ctr_cache_flush_instruction_range((void*)A9L_ADDR, (void*)(A9L_ADDR + bootloader_size));
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

