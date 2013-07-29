/**
  * Touhou Community Reliant Automatic Patcher
  * Main DLL
  *
  * ----
  *
  * Additional JSON-related functions.
  */

#pragma once

// Unfortunately, JSON doesn't support native hexadecimal values.
// This function takes both strings and integers and returns the
// correct number.
size_t json_hex_value(json_t *addr);

// Convert JSON string [object] to UTF-16.
// Return value has to be free()d by the caller!
wchar_t* json_string_value_utf16(const json_t *str_object);

// Get the integer value of [ind] in [array], automatically
// converting the JSON value to an integer if necessary.
size_t json_array_get_hex(json_t *array, const size_t ind);

// Convenience function for json_string_value(json_array_get(object, ind));
const char* json_array_get_string(const json_t *array, const size_t ind);

// Convert the [index]th value in [array] to UTF-16.
// Return value has to be free()d by the caller!
wchar_t* json_array_get_string_utf16(const json_t *array, const size_t ind);

// Same as json_object_get, but creates a [new_object] if the [key] doesn't exist
json_t* json_object_get_create(json_t *object, const char *key, json_t *new_object);

// Get the integer value of [key] in [object], automatically
// converting the JSON value to an integer if necessary.
size_t json_object_get_hex(json_t *object, const char *key);

// Convenience function for json_string_value(json_object_get(object, key));
const char* json_object_get_string(const json_t *object, const char *key);

// Convert the value of [key] in [object] to UTF-16.
// Return value has to be free()d by the caller!
wchar_t* json_object_get_string_utf16(const json_t *object, const char *key);

// Merge [src] recursively into [dest].
// [src] has priority; any element of [src] that is already present in [dest]
// and is *not* an object itself is overwritten.
int json_object_merge(json_t *dest, json_t *src);

// Return an alphabetically sorted JSON array of the keys in [object].
json_t* json_object_get_keys_sorted(const json_t *object);

// Wrapper around json_loadb and json_load_file with indirect UTF-8 filename
// support and nice error reporting.
// [source] can be specified to show the source of the JSON buffer in case of an error
// (since json_error_t::source will just say "<buffer>".
json_t* json_loadb_report(const void *buffer, size_t buflen, size_t flags, const char *source);
json_t* json_load_file_report(const char *json_fn);

// log_print for json_dump
int json_dump_log(const json_t *json, size_t flags);