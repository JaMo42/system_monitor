#include "globals.h"

unsigned page_shift_amount;
Proc_Map procs;
uint8_t current_generation;
Jiffy_List cpu_times;
unsigned long mem_total;
bool forest;
int sorting_mode;
VECTOR(Proc_Data*) sorted_procs;
bool show_kthreads;

