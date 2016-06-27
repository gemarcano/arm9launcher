#include "a9l_config.h"

#include <jsmn/jsmn.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

	for (size_t i = 0; i < config->num_entries; ++i)
	{
		a9l_config_entry_destroy(&config->entries[i]);
	}
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

void a9l_config_entry_initialize(a9l_config_entry *entry, const char *payload, size_t offset, ctr_hid_button_type buttons)
{
	entry->payload = malloc(strlen(payload) + 1);
	strcpy(entry->payload, payload);
	entry->offset = offset;
	entry->buttons = buttons;
}

void a9l_config_entry_destroy(a9l_config_entry *entry)
{
	free(entry->payload);
	entry->offset = 0;
	entry->buttons = 0;
}

static bool validate_json_tokens(const char* json, const jsmntok_t *tokens, size_t amount)
{
	size_t i = 0;
	//Main JSON object
	if (i >= amount || tokens[i].type != JSMN_OBJECT)
		return false;

	//"Should be "configuration" string
	if (++i >= amount ||
		tokens[i].type != JSMN_STRING ||
		strncmp("configuration", &json[tokens[i].start], 13) != 0)
		return false;

	//Array of entries
	if (++i >= amount || tokens[i].type != JSMN_ARRAY)
		return false;

	size_t array_size = (size_t)tokens[i].size;

	for (size_t j = 0; j < array_size; ++j)
	{
		//Should be the object wrapping an entry
		if (++i >= amount || tokens[i].type != JSMN_OBJECT)
			return false;

		//Should be "name"
		if (++i >= amount ||
			tokens[i].type != JSMN_STRING ||
			strncmp("name", &json[tokens[i].start], 4) != 0)
			return false;

		//This is the actual name of the entry
		if (++i >= amount || tokens[i].type != JSMN_STRING)
			return false;

		//Should be "location"
		if (++i >= amount ||
			tokens[i].type != JSMN_STRING ||
			strncmp("location", &json[tokens[i].start], 8) != 0)
			return false;

		//Should be the actual location of the entry
		if (++i >= amount || tokens[i].type != JSMN_STRING)
			return false;

		//Should be "offset"
		if (++i >= amount ||
			tokens[i].type != JSMN_STRING ||
			strncmp("offset", &json[tokens[i].start], 6) != 0)
			return false;

		//Should be a number
		if (++i >= amount || tokens[i].type != JSMN_PRIMITIVE)
			return false;

		//Should be "buttons"
		if (++i >= amount ||
			tokens[i].type != JSMN_STRING ||
			strncmp("buttons", &json[tokens[i].start], 7) != 0)
			return false;

		//Should be an array of buttons
		if (++i >= amount || tokens[i].type != JSMN_ARRAY)
			return false;

		size_t button_count = (size_t)tokens[i].size;
		for (size_t k = 0; k < button_count; ++k)
		{
			//Should be a string from a pre-determined set
			if (++i >= amount || tokens[i].type != JSMN_STRING)
				return false;
		}
	}

	return true;
}

static char *token_extract_string(const char *json, const jsmntok_t *token)
{
	size_t string_size = (size_t)token->end - (size_t)token->start;
	char *string = malloc(string_size + 1);
	memcpy(string, &json[token->start], string_size);
	string[string_size] = '\0';
	return string;
}

static long token_extract_long(const char *json, const jsmntok_t *token, char** end)
{
	return strtol(&json[token->start], end, 0);
}

static bool token_extract_button(const char *json, const jsmntok_t *token, ctr_hid_button_type *button)
{
	static const char *button_strings[] = {
		"None",
		"A",
		"B",
		"Select",
		"Start",
		"Right",
		"Left",
		"Up",
		"Down",
		"R",
		"L",
		"X",
		"Y"
	};

	const size_t button_strings_size = sizeof(button_strings)/sizeof(const char *);
	for (size_t i = 0; i < button_strings_size; i++)
	{
		const char *str = &json[token->start];
		if (strncmp(button_strings[i], str, strlen(button_strings[i])) == 0)
		{
			*button = (ctr_hid_button_type)((1 << i) >> 1);
			return true;
		}
	}
	return false;
}

static bool parse_json_tokens(const char* json, const jsmntok_t *tokens, size_t amount, a9l_config *config)
{
	size_t i = 2;
	size_t array_size = (size_t)tokens[i].size;
	a9l_config_initialize(config, array_size);

	for (size_t j = 0; j < array_size; ++j)
	{
		//This is the actual name of the entry
		i += 3;

		//Should be the actual location of the entry
		i += 2;
		char *payload = token_extract_string(json, &tokens[i]);

		//Should be a number
		i += 2;
		char *end = NULL;
		size_t offset = (size_t)token_extract_long(json, &tokens[i], &end);

		if (&json[tokens[i].end] != end)
		{
			a9l_config_destroy(config);
			free(payload);
			return false;
		}

		//Should be an array of buttons
		i += 2;

		size_t button_count = (size_t)tokens[i].size;
		ctr_hid_button_type buttons = 0;
		for (size_t k = 0; k < button_count; ++k)
		{
			//Should be a string from a pre-determined set
			++i;
			ctr_hid_button_type button;
			if (!token_extract_button(json, &tokens[i], &button))
			{
				a9l_config_destroy(config);
				free(payload);
				return false;
			}
			buttons |= button;
		}

		a9l_config_entry *entry = a9l_config_get_entry(config, j);
		a9l_config_entry_initialize(entry, payload, offset, buttons);
		free(payload);
	}

	return true;
}

bool a9l_config_read_json(a9l_config *config, const char *json)
{
	jsmn_parser parser;
	jsmntok_t *tokens;
	jsmn_init(&parser);

	int toks = jsmn_parse(&parser, json, strlen(json), NULL, 0);
	if (toks == JSMN_ERROR_INVAL)
	{
		return false;
	}

	tokens = malloc(sizeof(jsmntok_t) * (unsigned int)toks);

	jsmn_init(&parser);
	jsmn_parse(&parser, json, strlen(json), tokens, (unsigned int)toks);

	if (validate_json_tokens(json, tokens, (unsigned int)toks))
	{
		if (!parse_json_tokens(json, tokens, (unsigned int)toks, config))
		{
			a9l_config_destroy(config);
			return false;
		}
		return true;
	}

	return false;
}

