#pragma once
#include "stdafx.h"
#include "ini.h"

/// Reading a value stores it internally and return this struct which holds
/// functions to transform that stored value into the wanted type.
typedef struct {
  bool (*as_bool) ();
  const char * (*as_string) ();
  unsigned long (*as_unsigned) ();
  long (*as_signed) ();
} Config_Read_Value;

void ReadConfig ();

void FreeConfig ();

bool HaveConfig ();

Config_Read_Value * ConfigGet (const char *table, const char *name);
