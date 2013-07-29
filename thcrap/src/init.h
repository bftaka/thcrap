/**
  * Touhou Community Reliant Automatic Patcher
  * Main DLL
  *
  * ----
  *
  * DLL and engine initialization.
  */

#pragma once

int IsLatestBuild(const char *build, const char **latest, json_t *run_ver);

json_t* identify_by_hash(const char *fn, size_t *exe_size, json_t *versions);
json_t* identify_by_size(size_t exe_size, json_t *versions);

// Identifies the game, version and variety of [fn], 
// using hash and size lookup in versions.js.
// Also shows message boxes in case an unknown or outdated version was detected.
// Returns a fully merged run configuration on successful identification,
// NULL on failure or user cancellation.
json_t* identify(const char *fn);

int thcrap_init(const char *setup_fn);