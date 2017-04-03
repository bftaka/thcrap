/**
  * Touhou Community Reliant Automatic Patcher
  * Main DLL
  *
  * ----
  *
  * Wrapper around thcrap_update.dll function, providing a fallback if they are absent.
  */

#include "thcrap.h"
#include "thcrap_update_wrapper.h"

#define LOAD_ORIGINAL_FUNCTION(ret, name, ...) \
static ;

static FARPROC load_thcrap_update_function(const char* func_name)
{
	static HMODULE hMod = (HMODULE)-1;

	if (hMod == (HMODULE)-1) {
		hMod = LoadLibrary("thcrap_update.dll");
	}

	if (hMod) {
		return GetProcAddress(hMod, func_name);
	}
	return NULL;
}

#define CALL_WRAPPED_FUNCTION(cast, func_name, ...) \
static FARPROC cached_func = (FARPROC)-1; \
if (cached_func == (FARPROC)-1) { \
	cached_func = load_thcrap_update_function(func_name); \
} \
if (cached_func) { \
	return (cast cached_func)(__VA_ARGS__); \
}

int http_init_wrapper(void)
{
	CALL_WRAPPED_FUNCTION((int (*)(void)), "http_init")
	return 0;
}
void http_exit_wrapper(void)
{
	CALL_WRAPPED_FUNCTION((void (*)(void)), "http_exit")
}

json_t* ServerBuild_wrapper(const char *start_url)
{
	CALL_WRAPPED_FUNCTION((json_t *(*)(const char *)), "ServerBuild", start_url)
	return json_object();
}
json_t* ServerInit_wrapper(json_t *patch_js)
{
	CALL_WRAPPED_FUNCTION((json_t *(*)(json_t *)), "ServerInit", patch_js)
	return json_object();
}

void* ServerDownloadFile_wrapper(json_t *servers, const char *fn, DWORD *file_size, const DWORD *exp_crc)
{
	CALL_WRAPPED_FUNCTION((void *(*)(json_t *, const char *, DWORD *, const DWORD *)), "ServerDownloadFile", servers, fn, file_size, exp_crc)
	return NULL;
}

int update_filter_global_wrapper(const char *fn, json_t *null)
{
	CALL_WRAPPED_FUNCTION((int (*)(const char *, json_t *)), "update_filter_global", fn, null)
	return 0;
}
int update_filter_games_wrapper(const char *fn, json_t *games)
{
	CALL_WRAPPED_FUNCTION((int (*)(const char *, json_t *)), "update_filter_games", fn, games)
	return 0;
}
int patch_update_wrapper(json_t *patch_info, update_filter_func_t filter_func, json_t *filter_data)
{
	CALL_WRAPPED_FUNCTION((int (*)(json_t *, update_filter_func_t, json_t *)), "patch_update", patch_info, filter_func, filter_data)
	return 3;
}
void stack_update_wrapper(update_filter_func_t filter_func, json_t *filter_data)
{
	CALL_WRAPPED_FUNCTION((void (*)(update_filter_func_t, json_t *)), "stack_update", filter_func, filter_data)
}
