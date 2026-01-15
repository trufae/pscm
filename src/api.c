#include "api.h"
#include "reader.h"
#include "json.h"
#include <stdlib.h>
#include <string.h>

vm_t *scheme_create(void) {
    vm_t *vm = vm_create();
    if (!vm) return NULL;

    vm_register_builtins(vm);

    vm_register_native(vm, "json-parse", (value_t *(*)(vm_t *, value_t *))json_parse);
    vm_register_native(vm, "json-stringify", (value_t *(*)(vm_t *, value_t *))json_stringify);
    vm_register_native(vm, "json-select", (value_t *(*)(vm_t *, value_t *))json_select);

    return vm;
}

void scheme_destroy(vm_t *vm) {
    vm_destroy(vm);
}

int scheme_eval_string(vm_t *vm, const char *code, value_t **result) {
    if (!vm || !code) {
        if (vm) vm_set_error(vm, VERR_RUNTIME, "invalid arguments to eval_string");
        return 0;
    }

    scheme_clear_error(vm);

    reader_t *r = reader_create(vm, code);
    if (!r) {
        vm_set_error(vm, VERR_RUNTIME, "failed to create reader");
        return 0;
    }

    value_t *expr = reader_read(r);
    reader_destroy(r);

    if (!expr) {
        return 0;
    }

    value_t *val = vm_eval(vm, expr, vm->global_env);
    if (!val && vm_error_code(vm) != VERR_NONE) {
        return 0;
    }

    if (result) *result = val;
    return 1;
}

int scheme_has_error(vm_t *vm) {
    return vm_error_code(vm) != VERR_NONE;
}

const char *scheme_error_message(vm_t *vm) {
    return vm_error_message(vm);
}

void scheme_clear_error(vm_t *vm) {
    vm_clear_error(vm);
}

void scheme_interrupt(vm_t *vm) {
    vm_interrupt(vm);
}

value_t *scheme_make_null(vm_t *vm) {
    return value_null(vm);
}

value_t *scheme_make_bool(vm_t *vm, int b) {
    return value_bool(vm, b);
}

value_t *scheme_make_number(vm_t *vm, uint64_t n) {
    return value_number(vm, n);
}

value_t *scheme_make_double(vm_t *vm, double d) {
    return value_double(vm, d);
}

value_t *scheme_make_string(vm_t *vm, const char *s) {
    return value_string(vm, s);
}

value_t *scheme_make_list(vm_t *vm, value_t **items, size_t n) {
    value_t *list = value_null(vm);
    value_t **tail = &list;

    for (size_t i = 0; i < n; i++) {
        *tail = value_pair(vm, items[i], value_null(vm));
        tail = &((*tail)->as.pair.cdr);
    }

    return list;
}

value_t *scheme_make_vector(vm_t *vm, value_t **items, size_t n) {
    value_t *vec = value_vector(vm);
    if (!vec) return NULL;

    for (size_t i = 0; i < n; i++) {
        vector_push(vm, vec, items[i]);
    }

    return vec;
}

value_t *scheme_make_hash(vm_t *vm, value_t **keys, value_t **vals, size_t n) {
    value_t *hash = value_hash(vm);
    if (!hash) return NULL;

    for (size_t i = 0; i < n; i++) {
        hash_set(vm, hash, keys[i], vals[i]);
    }

    return hash;
}

int scheme_retain(value_t *v) {
    if (!v) return 0;
    value_retain(v);
    return 1;
}

int scheme_release(vm_t *vm, value_t *v) {
    if (!v) return 0;
    value_release(vm, v);
    return 1;
}

int scheme_is_null(value_t *v) { return value_is_null(v); }
int scheme_is_bool(value_t *v) { return value_is_bool(v); }
int scheme_is_number(value_t *v) { return value_is_number(v); }
int scheme_is_string(value_t *v) { return value_is_string(v); }
int scheme_is_vector(value_t *v) { return value_is_vector(v); }
int scheme_is_hash(value_t *v) { return value_is_hash(v); }
int scheme_is_pair(value_t *v) { return value_is_pair(v); }

int scheme_to_bool(value_t *v) { return value_to_bool(v); }

int scheme_to_number(value_t *v, uint64_t *out) {
    return value_to_number(v, out);
}

int scheme_to_double(value_t *v, double *out) {
    return value_to_double(v, out);
}

int scheme_to_string(value_t *v, const char **out) {
    return value_to_string(v, out);
}

int scheme_to_string_copy(value_t *v, char *buf, size_t len) {
    if (!value_is_string(v)) return 0;
    if (len > 0) strncpy(buf, v->as.string, len - 1);
    buf[len - 1] = '\0';
    return 1;
}

value_t *scheme_list_car(value_t *v) {
    if (!value_is_pair(v)) return NULL;
    return v->as.pair.car;
}

value_t *scheme_list_cdr(value_t *v) {
    if (!value_is_pair(v)) return NULL;
    return v->as.pair.cdr;
}

value_t *scheme_list_append(vm_t *vm, value_t *list, value_t *item) {
    if (!value_is_pair(list)) {
        return value_pair(vm, item, value_null(vm));
    }

    value_t *curr = list;
    while (value_is_pair(curr->as.pair.cdr)) {
        curr = curr->as.pair.cdr;
    }

    curr->as.pair.cdr = value_pair(vm, item, value_null(vm));
    return list;
}

size_t scheme_vector_len(value_t *v) {
    return vector_len(v);
}

value_t *scheme_vector_get(vm_t *vm, value_t *v, size_t index) {
    return vector_get(vm, v, index);
}

value_t *scheme_hash_get(vm_t *vm, value_t *hash, value_t *key) {
    return hash_get(vm, hash, key);
}

value_t *scheme_hash_set(vm_t *vm, value_t *hash, value_t *key, value_t *val) {
    return hash_set(vm, hash, key, val);
}

int scheme_register_native(vm_t *vm, const char *name, scheme_native_func func) {
    if (!vm || !name || !func) return 0;

    vm_register_native(vm, name, (value_t *(*)(vm_t *, value_t *))func);
    return 1;
}

value_t *scheme_call(vm_t *vm, const char *func_name, value_t **args, size_t nargs) {
    if (!vm || !func_name) {
        vm_set_error(vm, VERR_RUNTIME, "invalid arguments to call");
        return NULL;
    }

    value_t *sym = value_symbol(vm, func_name);
    value_t *func = vm_env_lookup(vm, vm->global_env, sym);

    if (!func) {
        vm_set_error(vm, VERR_UNBOUND, "function not found: %s", func_name);
        return NULL;
    }

    value_t *arg_list = value_null(vm);
    value_t **tail = &arg_list;

    for (size_t i = 0; i < nargs; i++) {
        *tail = value_pair(vm, args[i], value_null(vm));
        tail = &((*tail)->as.pair.cdr);
    }

    return vm_eval(vm, value_pair(vm, func, arg_list), vm->global_env);
}

int scheme_json_parse(vm_t *vm, const char *json_str, value_t **result) {
    if (!vm || !json_str || !result) {
        if (vm) vm_set_error(vm, VERR_RUNTIME, "invalid arguments to json_parse");
        return 0;
    }

    scheme_clear_error(vm);
    value_t *val = json_parse(vm, json_str);

    if (!val) {
        return 0;
    }

    *result = val;
    return 1;
}

int scheme_json_stringify(vm_t *vm, value_t *val, char *buf, size_t len) {
    if (!vm || !val || !buf || len == 0) {
        if (vm) vm_set_error(vm, VERR_RUNTIME, "invalid arguments to json_stringify");
        return 0;
    }

    scheme_clear_error(vm);
    value_t *str_val = json_stringify(vm, val);

    if (!str_val) {
        return 0;
    }

    if (value_is_string(str_val)) {
        strncpy(buf, str_val->as.string, len - 1);
        buf[len - 1] = '\0';
        value_release(vm, str_val);
        return 1;
    }

    value_release(vm, str_val);
    vm_set_error(vm, VERR_RUNTIME, "json_stringify returned non-string");
    return 0;
}