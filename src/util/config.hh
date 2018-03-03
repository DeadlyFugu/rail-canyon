// INI configuration I/O

#pragma once

#include "common.hh"

const char* config_get(const char* key, const char* def);
int config_geti(const char* key, int def);
float config_getf(const char* key, float def);

void config_set(const char* key, const char* value);
void config_seti(const char* key, int value);
void config_setf(const char* key, float value);
