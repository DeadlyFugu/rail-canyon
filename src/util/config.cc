#include "common.hh"

struct ConfigEntry {
	std::string key;
	std::string value;
};

static bool isConfigLoaded = false;
static std::vector<ConfigEntry> config;

static void config_load() {
	if (!isConfigLoaded) {
		isConfigLoaded = true;
		FILE* f = fopen("config.ini", "r");
		if (!f) return;

		fseek(f, 0, SEEK_END);
		auto len = ftell(f);
		fseek(f, 0, SEEK_SET);

		char* buffer = new char[len + 1];
		fread(buffer, len, 1, f);
		fclose(f);
		buffer[len] = '\0';

		char* p = buffer;
		while (*p) {
			char* lineBegin = p;
			char* colonPoint = 0;
			while (*p && *p != '\n') {
				if (!colonPoint && (*p == ':' || *p == '=')) colonPoint = p;
				p++;
			}
			char* lineEnd = p;
			if (*p == '\n') p++;
			if (!colonPoint) continue;

			config.emplace_back();
			auto& entry = config.back();

			// read key
			while (*lineBegin == ' ' || *lineBegin == '\t') lineBegin++;
			char* keyEnd = colonPoint - 1;
			while (*keyEnd == ' ' || *keyEnd == '\t') keyEnd--;
			keyEnd++;

			// put key
			if (keyEnd - lineBegin <= 0) {
				entry.key = "";
			} else {
				entry.key = std::string(lineBegin, keyEnd - lineBegin);
			}

			// read value
			colonPoint++;
			while (*colonPoint == ' ' || *colonPoint == '\t') colonPoint++;
			lineEnd--;
			while (*lineEnd == ' ' || *lineEnd == '\t') lineEnd--;
			lineEnd++;

			// put value
			if (lineEnd - colonPoint <= 0) {
				entry.value = "";
			} else {
				entry.value = std::string(colonPoint, lineEnd - colonPoint);
			}

			log_info("READ '%s': '%s'", entry.key.c_str(), entry.value.c_str());
		}

		delete[] buffer;
	}
}

static void config_save() {
	FILE* f = fopen("config.ini", "w");
	if (!f) {
		log_error("Could not write config.ini");
		return;
	}

	for (auto& entry : config) {
		fprintf(f, "%s : %s\n", entry.key.c_str(), entry.value.c_str());
	}

	fclose(f);

	log_info("Wrote config.ini");
}

const char* config_get(const char* key, const char* def) {
	config_load();
	std::string s_key(key);
	for (auto& entry : config) {
		if (entry.key == s_key) return entry.value.c_str();
	}
	return def;
}

int config_geti(const char* key, int def) {
	const char* result = config_get(key, 0);
	if (result) {
		int x;
		sscanf(result, "%d", &x);
		return x;
	}
	return def;
}

float config_getf(const char* key, float def) {
	const char* result = config_get(key, 0);
	if (result) {
		float x;
		sscanf(result, "%f", &x);
		return x;
	}
	return def;
}

void config_set(const char* key, const char* value) {
	config_load();
	std::string s_key(key);
	for (auto& entry : config) {
		if (entry.key == s_key) {
			entry.value = value;
			config_save();
			return;
		}
	}
	config.emplace_back();
	auto& entry = config.back();
	entry.key = key;
	entry.value = value;
	config_save();
}

void config_seti(const char* key, int value) {
	char buffer[64];
	snprintf(buffer, 64, "%d", value);
	config_set(key, buffer);
}

void config_setf(const char* key, float value) {
	char buffer[64];
	snprintf(buffer, 64, "%g", value);
	config_set(key, buffer);
}
