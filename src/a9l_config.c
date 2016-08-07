#include "a9l_config.h"

#include <jsmn/jsmn.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ARRAY_SIZE(X) (sizeof(X)/sizeof(*X))

typedef struct
{
	const char *name;
	bool mandatory;

} a9l_option;

static const a9l_option accepted_options[] =
{
	{"name", true },
	{"location", true },
	{"offset", false },
	{"buttons", true }
};

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

static bool check_match(const a9l_option list[], bool found_list[], size_t list_size, const char* string);
static bool check_all_mandatory_found(const a9l_option list[], bool found_list[], size_t list_size);
static bool validate_json_entry_token(const char* json, const jsmntok_t *tokens, size_t amount, size_t *current_element);
static bool validate_json_tokens(const char* json, const jsmntok_t *tokens, size_t amount);
static char *token_extract_string(const char *json, const jsmntok_t *token);
static long token_extract_long(const char *json, const jsmntok_t *token, char** end);
static bool token_extract_button(const char *json, const jsmntok_t *token, ctr_hid_button_type *button);
static bool parse_json_entry_token(const char* json, const jsmntok_t *tokens, size_t *current_element, a9l_config_entry *config_entry);
static bool parse_json_tokens(const char* json, const jsmntok_t *tokens, a9l_config *config);

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

	if (!validate_json_tokens(json, tokens, (unsigned int)toks))
	{
		free(tokens);
		return false;
	}
	
	if (!parse_json_tokens(json, tokens, config))
	{
		free(tokens);
		a9l_config_destroy(config);
		return false;
	}

	free(tokens);
	return true;
}


//Helper functions follow

static bool check_match(const a9l_option list[], bool found_list[], size_t list_size, const char* string)
{
	bool match = false;
	for (size_t i = 0; i < list_size && !match; ++i)
	{
		match = strncmp(list[i].name, string, strlen(list[i].name)) == 0;
		if (match)
		{
			found_list[i] = true;
		}
	}
	return match;
}

static bool check_all_mandatory_found(const a9l_option list[], bool found_list[], size_t list_size)
{
	bool found = true;
	for (size_t i = 0; i < list_size && found; ++i)
	{
		found = !list[i].mandatory || found_list[i];
	}
	return found;
}

static bool validate_json_entry_token(const char* json, const jsmntok_t *tokens, size_t amount, size_t *current_element)
{
	size_t i = *current_element;

	bool found_list[ARRAY_SIZE(accepted_options)] = { 0 };

	//begin checking

	//Should be the object wrapping an entry
	if (++i >= amount || tokens[i].type != JSMN_OBJECT)
		return false;

	size_t number_of_options = (size_t)tokens[i].size;
	if (number_of_options < 3)
		return false;

	for (size_t option = 0; option < number_of_options; ++option)
	{
		//Should be "name", "location", "offset". or "buttons"
		if (++i >= amount ||
			tokens[i].type != JSMN_STRING)
			return false;

		if (!check_match(accepted_options, found_list, ARRAY_SIZE(accepted_options), &json[tokens[i].start]))
			return false;

		if (strncmp("name", &json[tokens[i].start], 4) == 0)
		{
			//This is the actual name of the entry
			if (++i >= amount || tokens[i].type != JSMN_STRING)
				return false;

		}
		else if (strncmp("location", &json[tokens[i].start], 8) == 0)
		{
			//Should be the actual location of the entry
			if (++i >= amount || tokens[i].type != JSMN_STRING)
				return false;

		}
		else if (strncmp("buttons", &json[tokens[i].start], 7) == 0)
		{
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
		else //due to the earlier checks, this has to be offset
		{
			//Should be a number
			if (++i >= amount || tokens[i].type != JSMN_PRIMITIVE)
				return false;
		}
	}

	*current_element = i;

	return check_all_mandatory_found(accepted_options, found_list, ARRAY_SIZE(accepted_options));

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
		if (!validate_json_entry_token(json, tokens, amount, &i))
			return false;
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
	const size_t button_strings_size = sizeof(button_strings)/sizeof(const char *);
	for (size_t i = 0; i < button_strings_size; i++)
	{
		const char *str = &json[token->start];
		//FIXME Case insensitive?
		if (strncmp(button_strings[i], str, strlen(button_strings[i])) == 0)
		{
			*button = (ctr_hid_button_type)((1 << i) >> 1);
			return true;
		}
	}
	return false;
}

static bool parse_json_entry_token(const char* json, const jsmntok_t *tokens, size_t *current_element, a9l_config_entry *config_entry)
{
	size_t i = *current_element;

	//Mandatory entries: name, location, buttons(for now)
	//const char *name;
	char *location = NULL;
	ctr_hid_button_type buttons = CTR_HID_NONE;
	size_t offset = 0;

	size_t number_of_entries = (size_t)tokens[i++].size;

	//The token counter points at the beginning of the option pair at the
	//beginning of the for loop
	for (size_t entry = 0; entry < number_of_entries; ++entry, ++i)
	{
		const char *key = &json[tokens[i++].start];
		if (strncmp("name", key, 4) == 0)
		{
			//name = token_extract_string(json, &tokens[i]);
		}
		else if (strncmp("location", key, 8) == 0)
		{
			location = token_extract_string(json, &tokens[i]);
		}
		else if (strncmp("buttons", key, 7) == 0)
		{
			size_t button_count = (size_t)tokens[i++].size;
			for (size_t k = 0; k < button_count; ++k)
			{
				//Should be a string from a pre-determined set
				ctr_hid_button_type button;
				if (!token_extract_button(json, &tokens[i+k], &button))
				{
					free(location);
					return false;
				}
				buttons |= button;
			}
			if (button_count)
				i += (button_count-1);
		}
		else //due to the earlier checks, this has to be offset
		{
			char *end = NULL;
			offset = (size_t)token_extract_long(json, &tokens[i], &end);
			if (&json[tokens[i].end] != end)
			{
				free(location);
				return false;
			}
		}
	}

	a9l_config_entry_initialize(config_entry, location, offset, buttons);
	free(location);

	*current_element = i;

	return true;

}

static bool parse_json_tokens(const char* json, const jsmntok_t *tokens, a9l_config *config)
{
	//Skip the opening object/brace (0) and the "configuration" string (1)
	size_t i = 2;

	//Grab the number of entries in the entry array (2)
	//increment the counter to make sure to point to the first entry
	size_t array_size = (size_t)tokens[i++].size;

	a9l_config_initialize(config, array_size);

	for (size_t j = 0; j < array_size; ++j)
	{
		a9l_config_entry *entry = a9l_config_get_entry(config, j);
		if (!parse_json_entry_token(json, tokens, &i, entry))
		{
			return false;
		}
	}

	return true;
}

