#ifndef VM_H
#define VM_H

#include "value.h"

typedef enum {
    VERR_NONE,
    VERR_SYNTAX,
    VERR_RUNTIME,
    VERR_TYPE,
    VERR_ARGS,
    VERR_UNBOUND,
    VERR_INTERRUPTED,
} verror_t;

struct vm {
    value_t *global_env;
    verror_t error_code;
    char *error_message;
    int interrupt_flag;
};

vm_t *vm_create(void);
void vm_destroy(vm_t *vm);

void vm_set_error(vm_t *vm, verror_t code, const char *fmt, ...);
void vm_clear_error(vm_t *vm);
const char *vm_error_message(vm_t *vm);
verror_t vm_error_code(vm_t *vm);

void vm_interrupt(vm_t *vm);
int vm_check_interrupt(vm_t *vm);

value_t *vm_env_lookup(vm_t *vm, value_t *env, value_t *key);
value_t *vm_env_define(vm_t *vm, value_t *env, value_t *key, value_t *val);
value_t *vm_env_set(vm_t *vm, value_t *env, value_t *key, value_t *val);
value_t *vm_env_extend(vm_t *vm, value_t *env, value_t *keys, value_t *vals);

value_t *vm_eval(vm_t *vm, value_t *expr, value_t *env);

void vm_register_native(vm_t *vm, const char *name, value_t *(*func)(vm_t *, value_t *));
void vm_register_builtins(vm_t *vm);

#endif