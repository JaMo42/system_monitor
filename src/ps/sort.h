#pragma once
#include "../stdafx.h"
#include "ps.h"

void sort_procs (VECTOR(Proc_Data*) procs, int mode);

void sort_procs_recurse (VECTOR(Proc_Data*) procs, int mode);
