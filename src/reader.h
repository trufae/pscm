#ifndef READER_H
#define READER_H

#include "vm.h"
#include "value.h"

typedef struct reader {
    const char *input;
    size_t pos;
    size_t len;
    vm_t *vm;
} reader_t;

reader_t *reader_create(vm_t *vm, const char *input);
void reader_destroy(reader_t *r);

int reader_skip_whitespace(reader_t *r);
int reader_peek(reader_t *r);
int reader_next(reader_t *r);
int reader_match(reader_t *r, char c);

value_t *reader_read(reader_t *r);

value_t *read_string(reader_t *r);
value_t *read_number(reader_t *r);
value_t *read_symbol(reader_t *r);
value_t *read_list(reader_t *r, char end);
value_t *read_vector(reader_t *r);
value_t *read_hash(reader_t *r);

#endif