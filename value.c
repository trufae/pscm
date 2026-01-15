#include "value.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

value_t *value_alloc(vm_t *vm, vtype_t type) {
    value_t *v = calloc(1, sizeof(value_t));
    if (!v) return NULL;
    v->type = type;
    v->refcount = 1;
    return v;
}

value_t *value_null(vm_t *vm) {
    static value_t null_val = { VTYPE_NULL, 99999 };
    return &null_val;
}

value_t *value_bool(vm_t *vm, int b) {
    static value_t true_val = { VTYPE_BOOL, 99999, { .boolean = 1 } };
    static value_t false_val = { VTYPE_BOOL, 99999, { .boolean = 0 } };
    return b ? &true_val : &false_val;
}

value_t *value_number(vm_t *vm, uint64_t n) {
    value_t *v = value_alloc(vm, VTYPE_NUMBER);
    if (!v) return NULL;
    v->as.number = n;
    return v;
}

value_t *value_double(vm_t *vm, double d) {
    value_t *v = value_alloc(vm, VTYPE_NUMBER);
    if (!v) return NULL;
    v->as.floating = d;
    return v;
}

value_t *value_string(vm_t *vm, const char *s) {
    value_t *v = value_alloc(vm, VTYPE_STRING);
    if (!v) return NULL;
    v->as.string = strdup(s);
    return v;
}

value_t *value_symbol(vm_t *vm, const char *s) {
    value_t *v = value_alloc(vm, VTYPE_SYMBOL);
    if (!v) return NULL;
    v->as.symbol = strdup(s);
    return v;
}

value_t *value_pair(vm_t *vm, value_t *car, value_t *cdr) {
    value_t *v = value_alloc(vm, VTYPE_PAIR);
    if (!v) return NULL;
    v->as.pair.car = car;
    v->as.pair.cdr = cdr;
    if (car) value_retain(car);
    if (cdr) value_retain(cdr);
    return v;
}

value_t *value_vector(vm_t *vm) {
    value_t *v = value_alloc(vm, VTYPE_VECTOR);
    if (!v) return NULL;
    v->as.vector.elements = NULL;
    v->as.vector.size = 0;
    v->as.vector.capacity = 0;
    return v;
}

value_t *value_hash(vm_t *vm) {
    value_t *v = value_alloc(vm, VTYPE_HASH);
    if (!v) return NULL;
    v->as.hash.keys = NULL;
    v->as.hash.values = NULL;
    v->as.hash.size = 0;
    v->as.hash.capacity = 0;
    return v;
}

value_t *value_lambda(vm_t *vm, value_t *params, value_t *body, value_t *env) {
    value_t *v = value_alloc(vm, VTYPE_LAMBDA);
    if (!v) return NULL;
    v->as.lambda.params = params;
    v->as.lambda.body = body;
    v->as.lambda.env = env;
    if (params) value_retain(params);
    if (body) value_retain(body);
    if (env) value_retain(env);
    return v;
}

value_t *value_native(vm_t *vm, value_t *(*func)(vm_t *, value_t *)) {
    value_t *v = value_alloc(vm, VTYPE_NATIVE);
    if (!v) return NULL;
    v->as.native_func = func;
    return v;
}

void value_retain(value_t *v) {
    if (!v) return;
    v->refcount++;
}

void value_release(vm_t *vm, value_t *v) {
    if (!v || v->refcount == 99999) return;
    v->refcount--;
    if (v->refcount > 0) return;

    switch (v->type) {
        case VTYPE_STRING:
            free(v->as.string);
            break;
        case VTYPE_SYMBOL:
            free(v->as.symbol);
            break;
        case VTYPE_PAIR:
            value_release(vm, v->as.pair.car);
            value_release(vm, v->as.pair.cdr);
            break;
        case VTYPE_VECTOR:
            for (size_t i = 0; i < v->as.vector.size; i++) {
                value_release(vm, v->as.vector.elements[i]);
            }
            free(v->as.vector.elements);
            break;
        case VTYPE_HASH:
            for (size_t i = 0; i < v->as.hash.size; i++) {
                value_release(vm, v->as.hash.keys[i]);
                value_release(vm, v->as.hash.values[i]);
            }
            free(v->as.hash.keys);
            free(v->as.hash.values);
            break;
        case VTYPE_LAMBDA:
            value_release(vm, v->as.lambda.params);
            value_release(vm, v->as.lambda.body);
            value_release(vm, v->as.lambda.env);
            break;
        default:
            break;
    }
    free(v);
}

int value_equal(value_t *a, value_t *b) {
    if (a == b) return 1;
    if (!a || !b) return 0;
    if (a->type != b->type) return 0;

    switch (a->type) {
        case VTYPE_NULL:
            return 1;
        case VTYPE_BOOL:
            return a->as.boolean == b->as.boolean;
        case VTYPE_NUMBER:
            return a->as.number == b->as.number;
        case VTYPE_STRING:
            return strcmp(a->as.string, b->as.string) == 0;
        case VTYPE_SYMBOL:
            return strcmp(a->as.symbol, b->as.symbol) == 0;
        default:
            return 0;
    }
}

int value_is_null(value_t *v) { return v && v->type == VTYPE_NULL; }
int value_is_bool(value_t *v) { return v && v->type == VTYPE_BOOL; }
int value_is_number(value_t *v) { return v && v->type == VTYPE_NUMBER; }
int value_is_string(value_t *v) { return v && v->type == VTYPE_STRING; }
int value_is_symbol(value_t *v) { return v && v->type == VTYPE_SYMBOL; }
int value_is_pair(value_t *v) { return v && v->type == VTYPE_PAIR; }
int value_is_vector(value_t *v) { return v && v->type == VTYPE_VECTOR; }
int value_is_hash(value_t *v) { return v && v->type == VTYPE_HASH; }
int value_is_lambda(value_t *v) { return v && v->type == VTYPE_LAMBDA; }
int value_is_native(value_t *v) { return v && v->type == VTYPE_NATIVE; }
int value_is_callable(value_t *v) { return value_is_lambda(v) || value_is_native(v) || value_is_vector(v) || value_is_hash(v); }

int value_to_bool(value_t *v) {
    if (!v) return 0;
    if (value_is_bool(v)) return v->as.boolean;
    return !value_is_null(v);
}

int value_to_number(value_t *v, uint64_t *out) {
    if (!v || !value_is_number(v)) return 0;
    if (out) *out = v->as.number;
    return 1;
}

int value_to_double(value_t *v, double *out) {
    if (!v || !value_is_number(v)) return 0;
    if (out) *out = v->as.floating;
    return 1;
}

int value_to_string(value_t *v, const char **out) {
    if (!v || !value_is_string(v)) return 0;
    if (out) *out = v->as.string;
    return 1;
}

value_t *vector_push(vm_t *vm, value_t *vec, value_t *item) {
    if (!value_is_vector(vec)) return NULL;

    if (vec->as.vector.size >= vec->as.vector.capacity) {
        size_t new_cap = vec->as.vector.capacity == 0 ? 8 : vec->as.vector.capacity * 2;
        value_t **new_elems = realloc(vec->as.vector.elements, new_cap * sizeof(value_t *));
        if (!new_elems) return NULL;
        vec->as.vector.elements = new_elems;
        vec->as.vector.capacity = new_cap;
    }

    vec->as.vector.elements[vec->as.vector.size++] = item;
    value_retain(item);
    return vec;
}

value_t *vector_get(vm_t *vm, value_t *vec, size_t index) {
    if (!value_is_vector(vec) || index >= vec->as.vector.size) return NULL;
    return vec->as.vector.elements[index];
}

size_t vector_len(value_t *vec) {
    if (!value_is_vector(vec)) return 0;
    return vec->as.vector.size;
}

static size_t hash_find_slot(value_t *hash, value_t *key) {
    if (!value_is_hash(hash)) return (size_t)-1;
    if (hash->as.hash.capacity == 0) return (size_t)-1;

    uint64_t hash_val = 0;
    if (value_is_string(key)) {
        for (const char *s = key->as.string; *s; s++) {
            hash_val = hash_val * 31 + (uint8_t)*s;
        }
    } else if (value_is_symbol(key)) {
        for (const char *s = key->as.symbol; *s; s++) {
            hash_val = hash_val * 31 + (uint8_t)*s;
        }
    } else if (value_is_number(key)) {
        hash_val = key->as.number;
    } else {
        return (size_t)-1;
    }

    size_t idx = hash_val % hash->as.hash.capacity;
    size_t start_idx = idx;

    while (1) {
        if (hash->as.hash.keys[idx] && value_equal(hash->as.hash.keys[idx], key)) return idx;
        if (hash->as.hash.keys[idx] == NULL) return idx;

        idx = (idx + 1) % hash->as.hash.capacity;
        if (idx == start_idx) return (size_t)-1;
    }
}

value_t *hash_set(vm_t *vm, value_t *hash, value_t *key, value_t *val) {
    if (!value_is_hash(hash)) return NULL;

    if (hash->as.hash.size >= hash->as.hash.capacity * 3 / 4) {
        size_t old_cap = hash->as.hash.capacity;
        value_t **old_keys = hash->as.hash.keys;
        value_t **old_vals = hash->as.hash.values;
        size_t old_size = hash->as.hash.size;

        size_t new_cap = hash->as.hash.capacity == 0 ? 8 : hash->as.hash.capacity * 2;
        value_t **new_keys = calloc(new_cap, sizeof(value_t *));
        value_t **new_vals = calloc(new_cap, sizeof(value_t *));
        if (!new_keys || !new_vals) {
            free(new_keys);
            free(new_vals);
            return NULL;
        }

        hash->as.hash.keys = new_keys;
        hash->as.hash.values = new_vals;
        hash->as.hash.capacity = new_cap;
        hash->as.hash.size = 0;

        for (size_t i = 0; i < old_cap; i++) {
            if (old_keys[i]) {
                // reinsert
                value_t *key = old_keys[i];
                value_t *val = old_vals[i];
                size_t slot = hash_find_slot(hash, key);
                if (slot != (size_t)-1 && hash->as.hash.keys[slot] == NULL) {
                    hash->as.hash.keys[slot] = key;
                    hash->as.hash.values[slot] = val;
                    hash->as.hash.size++;
                }
            }
        }

        free(old_keys);
        free(old_vals);
    }

    size_t slot = hash_find_slot(hash, key);
    if (slot == (size_t)-1) {
        // table full, could resize here, but for now
        return NULL;
    }

    if (hash->as.hash.keys[slot] == NULL) {
        hash->as.hash.keys[slot] = key;
        value_retain(key);
        hash->as.hash.size++;
    } else {
        value_release(vm, hash->as.hash.values[slot]);
    }

    hash->as.hash.values[slot] = val;
    value_retain(val);
    return hash;
}

value_t *hash_get(vm_t *vm, value_t *hash, value_t *key) {
    if (!value_is_hash(hash)) return NULL;

    size_t slot = hash_find_slot(hash, key);
    if (slot == (size_t)-1 || !hash->as.hash.keys[slot]) return NULL;

    return hash->as.hash.values[slot];
}