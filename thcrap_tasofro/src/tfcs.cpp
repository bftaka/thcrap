/**
  * Touhou Community Reliant Automatic Patcher
  * Tasogare Frontier support plugin
  *
  * ----
  *
  * On-the-fly th145 TFCS patcher (*.csv)
  * Most CSV files can also be provided as TFCS files (and are provided by the game as TFCS files).
  * These CSV files aren't supported for now because the ULiL TFCS parser
  * is way more reliable than the ULiL CSV parser for these files.
  * If we add CSV support here, I think we'd better do it in the form of CSV=>TFCS conversion,
  * followed by TFCS patching.
  */

#include <thcrap.h>
#include "thcrap_tasofro.h"
#include "th135.h"
#include "tfcs.h"
#include "pl.h"
#include <zlib.h>
#include <vector>
#include <list>
#include <algorithm>

static int inflate_bytes(BYTE* file_in, size_t size_in, BYTE* file_out, size_t size_out)
{
	int ret;
	z_stream strm;

	strm.zalloc = NULL;
	strm.zfree = NULL;
	strm.opaque = NULL;
	strm.next_in = file_in;
	strm.avail_in = size_in;
	strm.next_out = file_out;
	strm.avail_out = size_out;
	ret = inflateInit(&strm);
	if (ret != Z_OK) {
		return ret;
	}
	do {
		ret = inflate(&strm, Z_NO_FLUSH);
		if (ret != Z_OK) {
			break;
		}
	} while (ret != Z_STREAM_END);
	inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : ret;
}

static int deflate_bytes(BYTE* file_in, size_t size_in, BYTE* file_out, size_t* size_out)
{
	int ret;
	z_stream strm;

	strm.zalloc = NULL;
	strm.zfree = NULL;
	strm.opaque = NULL;
	strm.next_in = file_in;
	strm.avail_in = size_in;
	strm.next_out = file_out;
	strm.avail_out = *size_out;
	ret = deflateInit(&strm, Z_BEST_COMPRESSION);
	if (ret != Z_OK) {
		return ret;
	}
	do {
		ret = deflate(&strm, Z_FINISH);
		if (ret != Z_OK) {
			break;
		}
	} while (ret != Z_STREAM_END);
	deflateEnd(&strm);
	*size_out -= strm.avail_out;
	return ret == Z_STREAM_END ? Z_OK : ret;
}

std::string patch_ruby(std::string str)
{
	const std::string ruby_begin = "{{ruby|";
	const std::string ruby_end = "}}";

	std::string::iterator pattern_begin;
	std::string::iterator pattern_end;
	while ((pattern_begin = std::search(str.begin(), str.end(), ruby_begin.begin(), ruby_begin.end())) != str.end()) {
		pattern_end = std::search(str.begin(), str.end(), ruby_end.begin(), ruby_end.end());
		if (pattern_end == str.end()) {
			break;
		}

		std::string rep = "\\R[";
		rep.append(pattern_begin + ruby_begin.length(), pattern_end);
		rep += "]";

		pattern_end += ruby_end.length();
		str.replace(pattern_begin, pattern_end, rep.begin(), rep.end());
	}

	return str;
}

static void patch_win_message(std::vector<std::string>& line, json_t *patch_row, size_t line_start,
	bool (*func)(std::vector<std::string>& line, TasofroPl::ALine* rep, size_t index))
{
	json_t *patch_lines = json_object_get(patch_row, "lines");
	if (patch_lines && line.size() >= line_start + 12 && line[line_start + 3].empty() == false) {
		std::list<TasofroPl::ALine*> texts;
		// We want to overwrite all the balloons with the user-provided ones,
		// so we only need to put the 1st one, we can ignore the others.
		TasofroPl::AText *text = new TasofroPl::WinText(std::vector<std::string>({
			line[line_start + 3],
			line[line_start + 2]
			}));
		texts.push_back(text);
		text->patch(texts, texts.begin(), "", patch_lines);

		size_t i = 0;
		for (TasofroPl::ALine* it : texts) {
			if (func(line, it, i) == false) {
				break;
			}
			i++;
		}
	}
}

void patch_line(BYTE *&in, BYTE *&out, DWORD nb_col, json_t *patch_row)
{
	// Read input
	std::vector<std::string> line;
	for (DWORD col = 0; col < nb_col; col++) {
		DWORD field_size = *(DWORD*)in;
		in += sizeof(DWORD);
		line.push_back(std::string((const char*)in, field_size));
		in += field_size;
	}

	// Patch as data/win/message/*.csv
	if (game_id <= TH145) {
		patch_win_message(line, patch_row, 1,
			[](std::vector<std::string>& line, TasofroPl::ALine* rep, size_t index) {
			if (index == 3) {
				log_print("TFCS: warning: trying to put more than 3 balloons in a win line.\n");
				return false;
			}
			line[1 + 4 * index + 3] = rep->get(0);
			line[1 + 4 * index + 2] = rep->get(1);
			return true;
		});
	}
	else {
		static json_t *subtitles_support = nullptr;
		static json_t *subtitles_config = nullptr;
		if (subtitles_support == nullptr) {
			subtitles_support = json_object_get(runconfig_get(), "subtitles_support");
		}
		if (subtitles_config == nullptr) {
			subtitles_config = json_object_get(runconfig_get(), "subtitles");
		}
		bool line_is_subtitle = json_is_true(json_object_get(patch_row, "is_subtitle"));

		auto patch_balloon_func = [](std::vector<std::string>& line, TasofroPl::ALine* rep, size_t index) {
			if (index == 3) {
				log_print("TFCS: warning: trying to put more than 3 balloons in a win line.\n");
				return false;
			}
			line[9 + 4 * index + 3] = rep->get(0);
			line[9 + 4 * index + 2] = rep->get(1);
			return true;
		};

		auto patch_subtitles_func = [](std::vector<std::string>& line, TasofroPl::ALine* rep, size_t index) {
			if (index == 2) {
				log_print("TFCS: warning: trying to put more than 2 subtitles in a win line.\n");
				return false;
			}
			line[21 + index] = rep->get(0);
			return true;
		};

		if (line_is_subtitle) {
			patch_win_message(line, patch_row, 9, patch_subtitles_func);
		}
		else if (json_is_true(subtitles_support) && json_is_true(subtitles_config)) {
			// If subtitles_config is true (which means it is NOT a patch stack, just the "true" value),
			// we don't have user-provided subtitles and we need to patch the subtitles with the main stack,
			// and to keep the balloon in Japanese.
			patch_win_message(line, patch_row, 9, patch_subtitles_func);
		}
		else {
			patch_win_message(line, patch_row, 9, patch_balloon_func);
		}
	}

	// Patch each column independently
	json_t *patch_col;
	for (DWORD col = 0; col < nb_col; col++) {
		patch_col = json_object_numkey_get(patch_row, col);
		if (patch_col) {
			if (json_is_string(patch_col)) {
				line[col] = patch_ruby(json_string_value(patch_col));
			}
			else if (json_is_array(patch_col)) {
				line[col].clear();
				bool add_eol = false;
				size_t i;
				json_t *it;
				json_array_foreach(patch_col, i, it) {
					if (add_eol) {
						line[col] += "\n";
					}
					if (json_is_string(it)) {
						line[col] += patch_ruby(json_string_value(it));
						add_eol = true;
					}
				}
			}
		}
	}

	// Write output
	for (const std::string& col : line) {
		DWORD field_size = col.length();
		*(DWORD*)out = field_size;
		out += sizeof(DWORD);
		memcpy(out, col.c_str(), field_size);
		out += field_size;
	}
}

void skip_line(BYTE *&in, BYTE *&out, DWORD nb_col)
{
	for (DWORD col = 0; col < nb_col; col++) {
		DWORD field_size = *(DWORD*)in; in += sizeof(DWORD);
		*(DWORD*)out = field_size;      out += sizeof(DWORD);

		memcpy(out, in, field_size);
		in  += field_size;
		out += field_size;
	}
}

int patch_tfcs(void *file_inout, size_t size_out, size_t size_in, const char *fn, json_t *patch)
{
	if (!patch) {
		return 0;
	}

	tfcs_header_t *header;

	// Read TFCS header
	header = (tfcs_header_t*)file_inout;
	if (size_in < sizeof(*header) || memcmp(header->magic, "TFCS\0", 5) != 0) {
		// Invalid TFCS file (probably a regular CSV file)
		return patch_csv(file_inout, size_out, size_in, fn, patch);
	}

	BYTE *file_in_uncomp = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, header->uncomp_size);
	inflate_bytes(header->data, header->comp_size, file_in_uncomp, header->uncomp_size);

	// We don't have patch_size here, but it is equal to size_out - size_in.
	DWORD file_out_uncomp_size = header->uncomp_size + (size_out - size_in);
	BYTE *file_out_uncomp = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, file_out_uncomp_size);

	BYTE *ptr_in  = file_in_uncomp;
	BYTE *ptr_out = file_out_uncomp;

	DWORD nb_row;
	DWORD nb_col;
	DWORD row;

	nb_row = *(DWORD*)ptr_in;  ptr_in  += sizeof(DWORD);
	*(DWORD*)ptr_out = nb_row; ptr_out += sizeof(DWORD);

	for (row = 0; row < nb_row; row++) {
		nb_col = *(DWORD*)ptr_in;  ptr_in  += sizeof(DWORD);
		*(DWORD*)ptr_out = nb_col; ptr_out += sizeof(DWORD);
		json_t *patch_row = json_object_numkey_get(patch, row);

		if (patch_row) {
			patch_line(ptr_in, ptr_out, nb_col, patch_row);
		}
		else {
			skip_line(ptr_in, ptr_out, nb_col);
		}
	}

	// Write result
	file_out_uncomp_size = ptr_out - file_out_uncomp;
	header->uncomp_size = file_out_uncomp_size;
	size_out -= sizeof(*header) - 1;
	int ret = deflate_bytes(file_out_uncomp, file_out_uncomp_size, header->data, &size_out);
	header->comp_size = size_out;
	if (ret != Z_OK) {
		log_printf("WARNING: tasofro CSV patching: compression failed with zlib error %d\n", ret);
	}

	HeapFree(GetProcessHeap(), 0, file_in_uncomp);
	HeapFree(GetProcessHeap(), 0, file_out_uncomp);

	return 1;
}

int patch_tfcs_subtitles(void *file_inout, size_t size_out, size_t size_in, const char *fn, json_t *)
{
	json_t *patch = custom_stack_game_json_resolve(fn, nullptr, json_object_get(runconfig_get(), "subtitles"));
	if (patch == nullptr) {
		return 0;
	}

	const char *key;
	json_t *value;
	json_object_foreach(patch, key, value) {
		if (json_is_object(value)) {
			json_object_set_new(value, "is_subtitle", json_true());
		}
	}

	int ret = patch_tfcs(file_inout, size_out, size_in, fn, patch);
	json_decref(patch);
	return ret;
}

size_t get_tfcs_size(const char*, json_t*, size_t patch_size)
{
	// Because a lot of these files are zipped, guessing their exact patched size is hard. We'll add a few more bytes.
	if (patch_size) {
		return (size_t)(patch_size * 1.2) + 2048 + 1;
	}
	else {
		return 0;
	}
}

size_t get_tfcs_subtitles_size(const char *fn, json_t*, size_t)
{
	size_t patch_size;
	json_t *patch = custom_stack_game_json_resolve(fn, &patch_size, json_object_get(runconfig_get(), "subtitles"));
	if (patch == nullptr) {
		return 0;
	}
	json_decref(patch);

	return get_tfcs_size(fn, nullptr, patch_size);
}
