#include "config.h"
#include <stdbool.h>

static Ini config_file;

static bool config_ok;

static const Ini_Table *config_current_table;

static const char *config_current_table_name = NULL;
static const char *config_current_name = NULL;

static Ini_String config_value_string;

static bool AsBool();
static const char *AsString();
static unsigned long AsUnsigned();
static long AsSigned();

static Config_Read_Value config_read_value_instance = {
    AsBool,
    AsString,
    AsUnsigned,
    AsSigned,
};

__attribute__((noreturn))
 static void
 ConfigInvalidValue() {
    fprintf(stderr, "config: invalid value for %s: %s\n", config_current_name,
            config_value_string.data);
    exit(1);
 }

 static bool AsBool() {
     if (strcasecmp(config_value_string.data, "true") == 0 ||
         strcasecmp(config_value_string.data, "yes") == 0 ||
         strcasecmp(config_value_string.data, "1") == 0) {
         return true;
     }
     if (strcasecmp(config_value_string.data, "false") == 0 ||
         strcasecmp(config_value_string.data, "no") == 0 ||
         strcasecmp(config_value_string.data, "0") == 0) {
         return false;
     }
     ConfigInvalidValue();
 }

 static const char *AsString() {
     return config_value_string.data;
 }

 static unsigned long AsUnsigned() {
     char *endptr;
     unsigned long value = strtoull(config_value_string.data, &endptr, 10);
     if (*endptr != '\0') {
         ConfigInvalidValue();
     }
     return value;
 }

 static long AsSigned() {
     char *endptr;
     long value = strtoll(config_value_string.data, &endptr, 10);
     if (*endptr != '\0') {
         ConfigInvalidValue();
     }
     return value;
 }

 bool TryReadConfig(Ini * out, const char **error, int *error_line) {
     const char *const home = getenv("HOME");
     const char *const path = "/.config/sm.ini";
     const size_t len = strlen(home) + strlen(path) + 1;
     char *pathname = malloc(len);
     snprintf(pathname, len, "%s%s", home, path);
     FILE *fp = fopen(pathname, "r");
     free(pathname);
     if (!fp) {
         return false;
     }
     Ini_Options options = INI_OPTIONS_WITH_FLAGS(INI_QUOTED_VALUES);
     Ini_Parse_Result result = ini_parse_file(fp, options);
     fclose(fp);
     if (result.ok) {
         *out = result.unwrap;
         return true;
     } else {
         *error = result.error;
         *error_line = result.error_line;
         return false;
     }
 }

 static void ClearGlobalState() {
     config_current_table = NULL;
     config_current_name = NULL;
     config_current_table_name = NULL;
     config_value_string.data = NULL;
     config_value_string.size = 0;
 }

 void ReadConfig() {
     config_ok = false;
     memset(&config_file, 0, sizeof(config_file));
     ClearGlobalState();
     const char *error;
     int error_line;
     if (!TryReadConfig(&config_file, &error, &error_line)) {
         fprintf(stderr, "Failed to parse config: %s on line %u\n", error, error_line);
         exit(1);
     }
     config_ok = true;
 }

 void FreeConfig() {
     ini_free(&config_file);
 }

 bool HaveConfig() {
     return config_ok;
 }

 Config_Read_Value *ConfigGet(const char *table, const char *name) {
     if (!config_current_table_name || strcmp(table, config_current_table_name) != 0) {
         config_current_table = ini_get_table(&config_file, table);
         config_current_table_name = table;
     }
     if (config_current_table == NULL) {
         return NULL;
     }
     config_value_string = ini_table_get(config_current_table, name);
     if (config_value_string.data == NULL) {
         return NULL;
     }
     config_current_name = name;
     return &config_read_value_instance;
 }

 void ConfigSet(Ini ini, bool ok, Ini * old, bool *old_ok) {
     if (old) {
         *old = config_file;
     }
     if (old_ok) {
         *old_ok = config_ok;
     }
     config_file = ini;
     config_ok = ok;
     ClearGlobalState();
 }
