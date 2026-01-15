#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>
#include <stddef.h>

typedef struct vm vm_t;

typedef enum {
    VTYPE_NULL,
    VTYPE_BOOL,
    VTYPE_NUMBER,
    VTYPE_STRING,
    VTYPE_SYMBOL,
    VTYPE_PAIR,
    VTYPE_VECTOR,
    VTYPE_HASH,
    VTYPE_LAMBDA,
    VTYPE_NATIVE,
} vtype_t;

typedef struct value {
    vtype_t type;
    int refcount;
    union {
        int boolean;
        uint64_t number;
        double floating;
        char *string;
        char *symbol;
        struct {
            struct value *car;
            struct value *cdr;
        } pair;
        struct {
            struct value **elements;
            size_t size;
            size_t capacity;
        } vector;
        struct {
            struct value **keys;
            struct value **values;
            size_t size;
            size_t capacity;
        } hash;
        struct {
            struct value *params;
            struct value *body;
            struct value *env;
        } lambda;
        struct value *(*native_func)(vm_t *vm, struct value *args);
    } as;
} value_t;

value_t *value_null(vm_t *vm);
value_t *value_bool(vm_t *vm, int b);
value_t *value_number(vm_t *vm, uint64_t n);
value_t *value_double(vm_t *vm, double d);
value_t *value_string(vm_t *vm, const char *s);
value_t *value_symbol(vm_t *vm, const char *s);
value_t *value_pair(vm_t *vm, value_t *car, value_t *cdr);
value_t *value_vector(vm_t *vm);
value_t *value_hash(vm_t *vm);
value_t *value_lambda(vm_t *vm, value_t *params, value_t *body, value_t *env);
value_t *value_native(vm_t *vm, value_t *(*func)(vm_t *, value_t *));

void value_retain(value_t *v);
void value_release(vm_t *vm, value_t *v);
int value_equal(value_t *a, value_t *b);

int value_is_null(value_t *v);
int value_is_bool(value_t *v);
int value_is_number(value_t *v);
int value_is_string(value_t *v);
int value_is_symbol(value_t *v);
int value_is_pair(value_t *v);
int value_is_vector(value_t *v);
int value_is_hash(value_t *v);
int value_is_lambda(value_t *v);
int value_is_native(value_t *v);
int value_is_callable(value_t *v);

int value_to_bool(value_t *v);
int value_to_number(value_t *v, uint64_t *out);
int value_to_double(value_t *v, double *out);
int value_to_string(value_t *v, const char **out);

value_t *vector_push(vm_t *vm, value_t *vec, value_t *item);
value_t *vector_get(vm_t *vm, value_t *vec, size_t index);
size_t vector_len(value_t *vec);

value_t *hash_set(vm_t *vm, value_t *hash, value_t *key, value_t *val);
value_t *hash_get(vm_t *vm, value_t *hash, value_t *key);

#endif