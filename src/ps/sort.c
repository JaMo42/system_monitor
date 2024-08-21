#include "sort.h"

// Compare function for qsort
typedef int (*Compare_Function)(const void *, const void *);

#define COMPARATOR(name_) int name_(const void *aa, const void *bb)

#define UNPACK() Proc_Data *a = *(Proc_Data **)aa, *b = *(Proc_Data **)bb

// If both values are the same, show most recent at the top
#define SUBSORT(x) ((x) == 0 ? (long)b->start_time - (long)a->start_time : (x))

COMPARATOR(compare_pid_descending) {
    UNPACK();
    return b->pid - a->pid;
}

COMPARATOR(compare_pid_ascending) {
    UNPACK();
    return a->pid - b->pid;
}

COMPARATOR(compare_cpu_descending) {
    UNPACK();
    return SUBSORT((intmax_t) b->total_cpu_time - (intmax_t) a->total_cpu_time);
}

COMPARATOR(compare_cpu_ascending) {
    UNPACK();
    return SUBSORT((intmax_t) a->total_cpu_time - (intmax_t) b->total_cpu_time);
}

COMPARATOR(compare_mem_descending) {
    UNPACK();
    return SUBSORT((long)b->total_memory - (long)a->total_memory);
}

COMPARATOR(compare_mem_ascending) {
    UNPACK();
    return SUBSORT((long)a->total_memory - (long)b->total_memory);
}

Compare_Function compare_functions[] = {
    compare_pid_descending, compare_pid_ascending, compare_cpu_descending,
    compare_cpu_ascending, compare_mem_descending, compare_mem_ascending
};

void
sort_procs(VECTOR(Proc_Data *)procs, int mode) {
    Compare_Function compare = compare_functions[mode];
    qsort(procs, vector_size(procs), sizeof(Proc_Data *), compare);
}

void
sort_procs_recurse(VECTOR(Proc_Data *)procs, int mode) {
    sort_procs(procs, mode);
    vector_for_each(procs, it) {
        if ((*it)->children) {
            sort_procs_recurse((*it)->children, mode);
        }
    }
}
