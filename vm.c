#include "vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

vm_t *vm_create(void) {
    vm_t *vm = calloc(1, sizeof(vm_t));
    if (!vm) return NULL;

    vm->global_env = value_hash(vm);
    if (!vm->global_env) {
        free(vm);
        return NULL;
    }
    value_retain(vm->global_env);

    return vm;
}

void vm_destroy(vm_t *vm) {
    if (!vm) return;

    value_release(vm, vm->global_env);
    free(vm->error_message);
    free(vm);
}

void vm_set_error(vm_t *vm, verror_t code, const char *fmt, ...) {
    if (!vm) return;

    free(vm->error_message);
    vm->error_code = code;

    va_list args;
    va_start(args, fmt);
    int ret = vasprintf(&vm->error_message, fmt, args);
    if (ret == -1) {
        vm->error_message = NULL;
    }
    va_end(args);
}

void vm_clear_error(vm_t *vm) {
    if (!vm) return;
    vm->error_code = VERR_NONE;
    free(vm->error_message);
    vm->error_message = NULL;
}

const char *vm_error_message(vm_t *vm) {
    return vm ? vm->error_message : NULL;
}

verror_t vm_error_code(vm_t *vm) {
    return vm ? vm->error_code : VERR_NONE;
}

void vm_interrupt(vm_t *vm) {
    if (vm) vm->interrupt_flag = 1;
}

int vm_check_interrupt(vm_t *vm) {
    if (!vm) return 0;
    if (vm->interrupt_flag) {
        vm_set_error(vm, VERR_INTERRUPTED, "execution interrupted");
        vm->interrupt_flag = 0;
        return 1;
    }
    return 0;
}

value_t *vm_env_lookup(vm_t *vm, value_t *env, value_t *key) {
    if (!value_is_hash(env)) return NULL;

    value_t *val = hash_get(vm, env, key);
    if (val) return val;

    return NULL;
}

value_t *vm_env_define(vm_t *vm, value_t *env, value_t *key, value_t *val) {
    if (!value_is_hash(env)) return NULL;
    return hash_set(vm, env, key, val);
}

value_t *vm_env_set(vm_t *vm, value_t *env, value_t *key, value_t *val) {
    return vm_env_define(vm, env, key, val);
}

value_t *vm_env_extend(vm_t *vm, value_t *env, value_t *keys, value_t *vals) {
    value_t *new_env = value_hash(vm);
    if (!new_env) return NULL;

    if (env) {
        for (size_t i = 0; i < env->as.hash.size; i++) {
            if (env->as.hash.keys[i]) {
                hash_set(vm, new_env, env->as.hash.keys[i], env->as.hash.values[i]);
            }
        }
    }

    value_t *k = keys;
    value_t *v = vals;
    while (!value_is_null(k) && !value_is_null(v)) {
        if (value_is_pair(k)) {
            vm_env_define(vm, new_env, k->as.pair.car, v->as.pair.car);
            k = k->as.pair.cdr;
            v = v->as.pair.cdr;
        } else {
            vm_env_define(vm, new_env, k, v);
            break;
        }
    }

    return new_env;
}

value_t *vm_eval(vm_t *vm, value_t *expr, value_t *env) {
    if (vm_check_interrupt(vm)) return NULL;

    if (!expr) return value_null(vm);

    if (!value_is_pair(expr)) {
        if (value_is_symbol(expr)) {
            value_t *val = vm_env_lookup(vm, env, expr);
            if (!val) {
                vm_set_error(vm, VERR_UNBOUND, "unbound symbol: %s", expr->as.symbol);
                return NULL;
            }
            return val;
        }
        return expr;
    }

    value_t *first = expr->as.pair.car;
    value_t *rest = expr->as.pair.cdr;

    if (value_is_symbol(first)) {
        const char *name = first->as.symbol;

        if (strcmp(name, "quote") == 0) {
            if (!value_is_pair(rest)) {
                vm_set_error(vm, VERR_ARGS, "quote: expected argument");
                return NULL;
            }
            return rest->as.pair.car;
        }

        if (strcmp(name, "if") == 0) {
            value_t *test = vm_eval(vm, rest->as.pair.car, env);
            if (!test) return NULL;

            value_t *then_expr = rest->as.pair.cdr->as.pair.car;
            value_t *else_expr = rest->as.pair.cdr->as.pair.cdr->as.pair.car;

            if (value_to_bool(test)) {
                return vm_eval(vm, then_expr, env);
            } else {
                return vm_eval(vm, else_expr, env);
            }
        }

        if (strcmp(name, "define") == 0) {
            if (!value_is_pair(rest)) {
                vm_set_error(vm, VERR_ARGS, "define: expected arguments");
                return NULL;
            }

            value_t *name_val = rest->as.pair.car;
            value_t *val_expr = rest->as.pair.cdr->as.pair.car;

            if (value_is_pair(name_val)) {
                value_t *func_name = name_val->as.pair.car;
                value_t *params = name_val->as.pair.cdr;
                value_t *body = rest->as.pair.cdr;

                value_t *lambda = value_lambda(vm, params, body, env);
                if (!lambda) return NULL;

                vm_env_define(vm, env, func_name, lambda);
                return func_name;
            } else {
                value_t *val = vm_eval(vm, val_expr, env);
                if (!val) return NULL;

                vm_env_define(vm, env, name_val, val);
                return name_val;
            }
        }

        if (strcmp(name, "lambda") == 0) {
            value_t *params = rest->as.pair.car;
            value_t *body = rest->as.pair.cdr;
            return value_lambda(vm, params, body, env);
        }

        if (strcmp(name, "let") == 0) {
            value_t *bindings = rest->as.pair.car;
            value_t *body = rest->as.pair.cdr;

            value_t *keys = value_null(vm);
            value_t *vals = value_null(vm);

            value_t *b = bindings;
            value_t **last_key = &keys;
            value_t **last_val = &vals;

            while (!value_is_null(b)) {
                value_t *binding = b->as.pair.car;
                value_t *k = binding->as.pair.car;
                value_t *v_expr = binding->as.pair.cdr->as.pair.car;
                value_t *v = vm_eval(vm, v_expr, env);
                if (!v) return NULL;

                *last_key = value_pair(vm, k, value_null(vm));
                *last_val = value_pair(vm, v, value_null(vm));
                last_key = &((*last_key)->as.pair.cdr);
                last_val = &((*last_val)->as.pair.cdr);

                b = b->as.pair.cdr;
            }

            value_t *new_env = vm_env_extend(vm, env, keys, vals);
            value_release(vm, keys);
            value_release(vm, vals);

            value_t *result = value_null(vm);
            value_t *expr_list = body;
            while (!value_is_null(expr_list)) {
                result = vm_eval(vm, expr_list->as.pair.car, new_env);
                if (!result) {
                    value_release(vm, new_env);
                    return NULL;
                }
                expr_list = expr_list->as.pair.cdr;
            }

            value_release(vm, new_env);
            return result;
        }
    }

    value_t *func = vm_eval(vm, first, env);
    if (!func) return NULL;

    if (vm_check_interrupt(vm)) return NULL;

    value_t *args = value_null(vm);
    value_t **last = &args;

    value_t *arg_exprs = rest;
    while (!value_is_null(arg_exprs)) {
        value_t *arg = vm_eval(vm, arg_exprs->as.pair.car, env);
        if (!arg) {
            value_release(vm, args);
            return NULL;
        }
        *last = value_pair(vm, arg, value_null(vm));
        last = &((*last)->as.pair.cdr);
        arg_exprs = arg_exprs->as.pair.cdr;
    }

    value_t *result = NULL;

    if (value_is_native(func)) {
        result = func->as.native_func(vm, args);
    } else if (value_is_lambda(func)) {
        value_t *new_env = vm_env_extend(vm, func->as.lambda.env, func->as.lambda.params, args);
        value_release(vm, args);

        value_t *expr_list = func->as.lambda.body;
        value_t *last_result = value_null(vm);
        while (!value_is_null(expr_list)) {
            last_result = vm_eval(vm, expr_list->as.pair.car, new_env);
            if (!last_result) {
                value_release(vm, new_env);
                return NULL;
            }
            expr_list = expr_list->as.pair.cdr;
        }

        value_release(vm, new_env);
        result = last_result;
    } else if (value_is_vector(func)) {
        value_t *index = args->as.pair.car;
        if (!value_is_number(index)) {
            vm_set_error(vm, VERR_TYPE, "vector index must be number");
            value_release(vm, args);
            return NULL;
        }
        result = vector_get(vm, func, (size_t)index->as.number);
        if (!result) {
            vm_set_error(vm, VERR_RUNTIME, "vector index out of bounds");
        }
        value_release(vm, args);
    } else if (value_is_hash(func)) {
        value_t *key = args->as.pair.car;
        result = hash_get(vm, func, key);
        if (!result) {
            vm_set_error(vm, VERR_RUNTIME, "hash key not found");
        }
        value_release(vm, args);
    } else {
        vm_set_error(vm, VERR_TYPE, "not callable");
        value_release(vm, args);
        return NULL;
    }

    return result;
}

void vm_register_native(vm_t *vm, const char *name, value_t *(*func)(vm_t *, value_t *)) {
    value_t *sym = value_symbol(vm, name);
    value_t *native = value_native(vm, func);
    vm_env_define(vm, vm->global_env, sym, native);
}