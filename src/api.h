#ifndef API_H
#define API_H

#include "vm.h"
#include "value.h"

vm_t *scheme_create(void);
void scheme_destroy(vm_t *vm);

int scheme_eval_string(vm_t *vm, const char *code, value_t **result);

int scheme_has_error(vm_t *vm);
const char *scheme_error_message(vm_t *vm);
void scheme_clear_error(vm_t *vm);

void scheme_interrupt(vm_t *vm);

value_t *scheme_make_null(vm_t *vm);
value_t *scheme_make_bool(vm_t *vm, int b);
value_t *scheme_make_number(vm_t *vm, uint64_t n);
value_t *scheme_make_double(vm_t *vm, double d);
value_t *scheme_make_string(vm_t *vm, const char *s);

value_t *scheme_make_list(vm_t *vm, value_t **items, size_t n);
value_t *scheme_make_vector(vm_t *vm, value_t **items, size_t n);
value_t *scheme_make_hash(vm_t *vm, value_t **keys, value_t **vals, size_t n);

int scheme_retain(value_t *v);
int scheme_release(vm_t *vm, value_t *v);

int scheme_is_null(value_t *v);
int scheme_is_bool(value_t *v);
int scheme_is_number(value_t *v);
int scheme_is_string(value_t *v);
int scheme_is_vector(value_t *v);
int scheme_is_hash(value_t *v);
int scheme_is_pair(value_t *v);

int scheme_to_bool(value_t *v);
int scheme_to_number(value_t *v, uint64_t *out);
int scheme_to_double(value_t *v, double *out);
int scheme_to_string(value_t *v, const char **out);
int scheme_to_string_copy(value_t *v, char *buf, size_t len);

value_t *scheme_list_car(value_t *v);
value_t *scheme_list_cdr(value_t *v);
value_t *scheme_list_append(vm_t *vm, value_t *list, value_t *item);

size_t scheme_vector_len(value_t *v);
value_t *scheme_vector_get(vm_t *vm, value_t *v, size_t index);

value_t *scheme_hash_get(vm_t *vm, value_t *hash, value_t *key);
value_t *scheme_hash_set(vm_t *vm, value_t *hash, value_t *key, value_t *val);

typedef value_t *(*scheme_native_func)(vm_t *vm, value_t *args);
int scheme_register_native(vm_t *vm, const char *name, scheme_native_func func);

value_t *scheme_call(vm_t *vm, const char *func_name, value_t **args, size_t nargs);

int scheme_json_parse(vm_t *vm, const char *json_str, value_t **result);
int scheme_json_stringify(vm_t *vm, value_t *val, char *buf, size_t len);

#endif