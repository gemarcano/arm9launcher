#ifndef A9L_CONFIG_H_
#define A9L_CONFIG_H_

#include <ctr9/ctr_hid.h>
#include <stddef.h>

typedef struct
{
	const char *payload;
	size_t offset;
	ctr_hid_button_type buttons;
} a9l_config_entry;

typedef struct
{
	a9l_config_entry *entries;
	size_t num_entries;
} a9l_config;

void a9l_config_initialize(a9l_config *config, size_t entries);
void a9l_config_destroy(a9l_config *config);

a9l_config_entry* a9l_config_get_entry(const a9l_config *config, size_t entry);

size_t a9l_config_get_number_of_entries(const a9l_config *config);

#endif//A9L_CONFIG_H_

