#include "a9l_config.h"
#include <ctr9/ctr_hid.h>

#include <stdlib.h>

void a9l_config_initialize(a9l_config *config, size_t entries)
{
	config->entries = malloc(sizeof(a9l_config_entry) * entries);
	if (config->entries)
	{
		config->num_entries = entries;
		for (size_t i = 0; i < entries; ++i)
		{
			config->entries[i].payload = NULL;
			config->entries[i].offset = 0;
			config->entries[i].buttons = 0;
		}
	}
}

void a9l_config_destroy(a9l_config *config)
{
	free(config->entries);
	config->entries = NULL;
	config->num_entries = 0;
}

a9l_config_entry* a9l_config_get_entry(const a9l_config *config, size_t entry)
{
	return &(config->entries[entry]);
}

size_t a9l_config_get_number_of_entries(const a9l_config *config)
{
	return config->num_entries;
}

