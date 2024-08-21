#include "jiffy_list.h"

#define IDX(x) ((x) - JIFFY_LIST_SIZE * ((x) == JIFFY_LIST_SIZE))

void
jiffy_list_construct(Jiffy_List *self, unsigned long initial_sample) {
    for (int i = 0; i < JIFFY_LIST_SIZE; ++i) {
        self->data[i] = initial_sample;
    }
    self->begin = 0;
}

void
jiffy_list_push(Jiffy_List *self, unsigned long sample) {
    self->begin = IDX(self->begin + 1);
    self->data[self->begin] = sample;
}

unsigned long
jiffy_list_period(Jiffy_List *self) {
    unsigned long first = self->data[self->begin];
    unsigned long last = self->data[IDX(self->begin + 1)];
    return first - last;
}
