#include "config.h"

static Ini config_file;

static bool config_ok;

const Ini_Table *config_current_table;

const char *config_current_table_name = NULL;
const char *config_current_name;

Ini_String config_value_string;

static bool AsBool ();
static const char * AsString ();
static unsigned long AsUnsigned ();

static Config_Read_Value config_read_value_instance = {
  AsBool,
  AsString,
  AsUnsigned
};

__attribute__ ((noreturn)) static void
ConfigInvalidValue ()
{
  fprintf (stderr, "config: invalid value for %s: %s\n",
           config_current_name, config_value_string.data);
  exit (1);
}

static bool
AsBool ()
{
  if (strcasecmp (config_value_string.data, "true") == 0
      || strcasecmp (config_value_string.data, "yes") == 0
      || strcasecmp (config_value_string.data, "1") == 0)
    return true;
  if (strcasecmp (config_value_string.data, "true") == 0
      || strcasecmp (config_value_string.data, "yes") == 0
      || strcasecmp (config_value_string.data, "1") == 0)
    return false;
  ConfigInvalidValue ();
}

static const char *
AsString ()
{
  return config_value_string.data;
}

static unsigned long
AsUnsigned ()
{
  char *endptr;
  unsigned long value = strtoull (config_value_string.data, &endptr, 10);
  if (*endptr != '\0')
    ConfigInvalidValue ();
  return value;
}

void
ReadConfig ()
{
  const char *const home = getenv ("HOME");
  const char *const path = "/.config/sm.ini";
  const size_t len = strlen (home) + strlen (path) + 1;
  char *pathname = malloc (len);
  strncpy (pathname, home, len);
  strncat (pathname, path, len);
  FILE *fp = fopen (pathname, "r");
  free (pathname);
  if (!fp) {
    config_ok = false;
    return;
  }
  Ini_Options options = INI_OPTIONS_WITH_FLAGS (INI_QUOTED_VALUES);
  Ini_Parse_Result result = ini_parse_file (fp, options);
  fclose (fp);
  if (result.ok) {
    config_ok = true;
    config_file = result.unwrap;
  } else {
    fprintf (stderr, "Failed to parse config: %s on line %u\n",
             result.error, result.error_line);
    exit (1);
  }
}

void
FreeConfig ()
{
  ini_free (&config_file);
}

bool
HaveConfig ()
{
  return config_ok;
}

Config_Read_Value *
ConfigGet (const char *table, const char *name)
{
  if (!config_current_table_name
      || strcmp (table, config_current_table_name) != 0) {
    config_current_table = ini_get_table (&config_file, table);
    config_current_table_name = table;
  }
  if (config_current_table == NULL)
    return NULL;
  config_value_string = ini_table_get (config_current_table, name);
  if (config_value_string.data == NULL)
    return NULL;
  return &config_read_value_instance;
}
