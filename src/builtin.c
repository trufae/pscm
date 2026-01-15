#include "vm.h"
#include "value.h"
#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static value_t *builtin_add(vm_t *vm, value_t *args) {
    uint64_t result = 0;
    double result_d = 0.0;
    int has_float = 0;

    value_t *arg = args;
    while (!value_is_null(arg)) {
        value_t *val = arg->as.pair.car;
        if (!value_is_number(val)) {
            vm_set_error(vm, VERR_TYPE, "+: expected number");
            return NULL;
        }
        if (has_float) {
            result_d += val->as.floating;
        } else {
            result += val->as.number;
            if (val->as.number != (double)val->as.number) {
                result_d = (double)result + val->as.floating - val->as.number;
                has_float = 1;
            }
        }
        arg = arg->as.pair.cdr;
    }

    if (has_float) {
        return value_double(vm, result_d);
    }
    return value_number(vm, result);
}

static value_t *builtin_sub(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "-: expected at least 1 argument");
        return NULL;
    }

    value_t *first = args->as.pair.car;
    if (!value_is_number(first)) {
        vm_set_error(vm, VERR_TYPE, "-: expected number");
        return NULL;
    }

    double result = first->as.floating;
    uint64_t result_i = first->as.number;
    int has_float = (first->as.number != (double)first->as.number);

    value_t *arg = args->as.pair.cdr;
    while (!value_is_null(arg)) {
        value_t *val = arg->as.pair.car;
        if (!value_is_number(val)) {
            vm_set_error(vm, VERR_TYPE, "-: expected number");
            return NULL;
        }
        if (has_float) {
            result -= val->as.floating;
        } else {
            result_i -= val->as.number;
            if (val->as.number != (double)val->as.number) {
                result = (double)result_i - val->as.floating + val->as.number;
                has_float = 1;
            }
        }
        arg = arg->as.pair.cdr;
    }

    if (has_float) {
        return value_double(vm, result);
    }
    return value_number(vm, result_i);
}

static value_t *builtin_mul(vm_t *vm, value_t *args) {
    uint64_t result = 1;
    double result_d = 1.0;
    int has_float = 0;

    value_t *arg = args;
    while (!value_is_null(arg)) {
        value_t *val = arg->as.pair.car;
        if (!value_is_number(val)) {
            vm_set_error(vm, VERR_TYPE, "*: expected number");
            return NULL;
        }
        if (has_float) {
            result_d *= val->as.floating;
        } else {
            result *= val->as.number;
            if (val->as.number != (double)val->as.number) {
                result_d = (double)result * val->as.floating / val->as.number;
                has_float = 1;
            }
        }
        arg = arg->as.pair.cdr;
    }

    if (has_float) {
        return value_double(vm, result_d);
    }
    return value_number(vm, result);
}

static value_t *builtin_div(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "/: expected at least 1 argument");
        return NULL;
    }

    value_t *first = args->as.pair.car;
    if (!value_is_number(first)) {
        vm_set_error(vm, VERR_TYPE, "/: expected number");
        return NULL;
    }

    double result = first->as.floating;

    value_t *arg = args->as.pair.cdr;
    while (!value_is_null(arg)) {
        value_t *val = arg->as.pair.car;
        if (!value_is_number(val)) {
            vm_set_error(vm, VERR_TYPE, "/: expected number");
            return NULL;
        }
        if (val->as.floating == 0.0) {
            vm_set_error(vm, VERR_RUNTIME, "/: division by zero");
            return NULL;
        }
        result /= val->as.floating;
        arg = arg->as.pair.cdr;
    }

    return value_double(vm, result);
}

static value_t *builtin_eq(vm_t *vm, value_t *args) {
    if (value_is_null(args) || value_is_null(args->as.pair.cdr)) {
        vm_set_error(vm, VERR_ARGS, "=: expected at least 2 arguments");
        return NULL;
    }

    value_t *first = args->as.pair.car;
    value_t *arg = args->as.pair.cdr;

    while (!value_is_null(arg)) {
        value_t *val = arg->as.pair.car;
        if (!value_equal(first, val)) {
            return value_bool(vm, 0);
        }
        arg = arg->as.pair.cdr;
    }

    return value_bool(vm, 1);
}

static value_t *builtin_lt(vm_t *vm, value_t *args) {
    if (value_is_null(args) || value_is_null(args->as.pair.cdr)) {
        vm_set_error(vm, VERR_ARGS, "<: expected at least 2 arguments");
        return NULL;
    }

    value_t *first = args->as.pair.car;
    if (!value_is_number(first)) {
        vm_set_error(vm, VERR_TYPE, "<: expected number");
        return NULL;
    }

    value_t *arg = args->as.pair.cdr;
    while (!value_is_null(arg)) {
        value_t *val = arg->as.pair.car;
        if (!value_is_number(val)) {
            vm_set_error(vm, VERR_TYPE, "<: expected number");
            return NULL;
        }
        if (first->as.floating >= val->as.floating) {
            return value_bool(vm, 0);
        }
        first = val;
        arg = arg->as.pair.cdr;
    }

    return value_bool(vm, 1);
}

static value_t *builtin_gt(vm_t *vm, value_t *args) {
    if (value_is_null(args) || value_is_null(args->as.pair.cdr)) {
        vm_set_error(vm, VERR_ARGS, ">: expected at least 2 arguments");
        return NULL;
    }

    value_t *first = args->as.pair.car;
    if (!value_is_number(first)) {
        vm_set_error(vm, VERR_TYPE, ">: expected number");
        return NULL;
    }

    value_t *arg = args->as.pair.cdr;
    while (!value_is_null(arg)) {
        value_t *val = arg->as.pair.car;
        if (!value_is_number(val)) {
            vm_set_error(vm, VERR_TYPE, ">: expected number");
            return NULL;
        }
        if (first->as.floating <= val->as.floating) {
            return value_bool(vm, 0);
        }
        first = val;
        arg = arg->as.pair.cdr;
    }

    return value_bool(vm, 1);
}

static value_t *builtin_cons(vm_t *vm, value_t *args) {
    if (value_is_null(args) || value_is_null(args->as.pair.cdr)) {
        vm_set_error(vm, VERR_ARGS, "cons: expected 2 arguments");
        return NULL;
    }
    return value_pair(vm, args->as.pair.car, args->as.pair.cdr->as.pair.car);
}

static value_t *builtin_car(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "car: expected 1 argument");
        return NULL;
    }
    value_t *pair = args->as.pair.car;
    if (!value_is_pair(pair)) {
        vm_set_error(vm, VERR_TYPE, "car: expected pair");
        return NULL;
    }
    return pair->as.pair.car;
}

static value_t *builtin_cdr(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "cdr: expected 1 argument");
        return NULL;
    }
    value_t *pair = args->as.pair.car;
    if (!value_is_pair(pair)) {
        vm_set_error(vm, VERR_TYPE, "cdr: expected pair");
        return NULL;
    }
    return pair->as.pair.cdr;
}

static value_t *builtin_list(vm_t *vm, value_t *args) {
    return args;
}

static value_t *builtin_null_p(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "null?: expected 1 argument");
        return NULL;
    }
    return value_bool(vm, value_is_null(args->as.pair.car));
}

static value_t *builtin_pair_p(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "pair?: expected 1 argument");
        return NULL;
    }
    return value_bool(vm, value_is_pair(args->as.pair.car));
}

static value_t *builtin_number_p(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "number?: expected 1 argument");
        return NULL;
    }
    return value_bool(vm, value_is_number(args->as.pair.car));
}

static value_t *builtin_string_p(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "string?: expected 1 argument");
        return NULL;
    }
    return value_bool(vm, value_is_string(args->as.pair.car));
}

static value_t *builtin_symbol_p(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "symbol?: expected 1 argument");
        return NULL;
    }
    return value_bool(vm, value_is_symbol(args->as.pair.car));
}

static value_t *builtin_vector_p(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "vector?: expected 1 argument");
        return NULL;
    }
    return value_bool(vm, value_is_vector(args->as.pair.car));
}

static value_t *builtin_hash_p(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "hash?: expected 1 argument");
        return NULL;
    }
    return value_bool(vm, value_is_hash(args->as.pair.car));
}

static value_t *builtin_vector(vm_t *vm, value_t *args) {
    value_t *vec = value_vector(vm);
    value_t *arg = args;
    while (!value_is_null(arg)) {
        vector_push(vm, vec, arg->as.pair.car);
        arg = arg->as.pair.cdr;
    }
    return vec;
}

static value_t *builtin_hash(vm_t *vm, value_t *args) {
    value_t *hash = value_hash(vm);
    value_t *arg = args;
    while (!value_is_null(arg)) {
        value_t *pair = arg->as.pair.car;
        if (!value_is_pair(pair) || value_is_null(pair->as.pair.cdr) || !value_is_null(pair->as.pair.cdr->as.pair.cdr)) {
            vm_set_error(vm, VERR_ARGS, "hash: expected key-value pairs");
            value_release(vm, hash);
            return NULL;
        }
        hash_set(vm, hash, pair->as.pair.car, pair->as.pair.cdr->as.pair.car);
        arg = arg->as.pair.cdr;
    }
    return hash;
}

static value_t *builtin_hash_set(vm_t *vm, value_t *args) {
    if (value_is_null(args) || value_is_null(args->as.pair.cdr) || value_is_null(args->as.pair.cdr->as.pair.cdr)) {
        vm_set_error(vm, VERR_ARGS, "hash-set!: expected 3 arguments");
        return NULL;
    }
    value_t *hash = args->as.pair.car;
    value_t *key = args->as.pair.cdr->as.pair.car;
    value_t *val = args->as.pair.cdr->as.pair.cdr->as.pair.car;

    if (!value_is_hash(hash)) {
        vm_set_error(vm, VERR_TYPE, "hash-set!: expected hash");
        return NULL;
    }

    return hash_set(vm, hash, key, val);
}

static value_t *builtin_hash_ref(vm_t *vm, value_t *args) {
    if (value_is_null(args) || value_is_null(args->as.pair.cdr)) {
        vm_set_error(vm, VERR_ARGS, "hash-ref: expected 2 arguments");
        return NULL;
    }
    value_t *hash = args->as.pair.car;
    value_t *key = args->as.pair.cdr->as.pair.car;

    if (!value_is_hash(hash)) {
        vm_set_error(vm, VERR_TYPE, "hash-ref: expected hash");
        return NULL;
    }

    value_t *val = hash_get(vm, hash, key);
    if (!val) {
        return value_null(vm);
    }
    return val;
}

static value_t *builtin_vector_ref(vm_t *vm, value_t *args) {
    if (value_is_null(args) || value_is_null(args->as.pair.cdr)) {
        vm_set_error(vm, VERR_ARGS, "vector-ref: expected 2 arguments");
        return NULL;
    }
    value_t *vec = args->as.pair.car;
    value_t *index = args->as.pair.cdr->as.pair.car;

    if (!value_is_vector(vec)) {
        vm_set_error(vm, VERR_TYPE, "vector-ref: expected vector");
        return NULL;
    }
    if (!value_is_number(index)) {
        vm_set_error(vm, VERR_TYPE, "vector-ref: expected number index");
        return NULL;
    }

    value_t *val = vector_get(vm, vec, (size_t)index->as.number);
    if (!val) {
        vm_set_error(vm, VERR_RUNTIME, "vector-ref: index out of bounds");
        return NULL;
    }
    return val;
}

static value_t *builtin_vector_set(vm_t *vm, value_t *args) {
    if (value_is_null(args) || value_is_null(args->as.pair.cdr) || value_is_null(args->as.pair.cdr->as.pair.cdr)) {
        vm_set_error(vm, VERR_ARGS, "vector-set!: expected 3 arguments");
        return NULL;
    }
    value_t *vec = args->as.pair.car;
    value_t *index = args->as.pair.cdr->as.pair.car;
    value_t *val = args->as.pair.cdr->as.pair.cdr->as.pair.car;

    if (!value_is_vector(vec)) {
        vm_set_error(vm, VERR_TYPE, "vector-set!: expected vector");
        return NULL;
    }
    if (!value_is_number(index)) {
        vm_set_error(vm, VERR_TYPE, "vector-set!: expected number index");
        return NULL;
    }

    if ((size_t)index->as.number >= vec->as.vector.size) {
        vm_set_error(vm, VERR_RUNTIME, "vector-set!: index out of bounds");
        return NULL;
    }

    value_release(vm, vec->as.vector.elements[index->as.number]);
    vec->as.vector.elements[index->as.number] = val;
    value_retain(val);
    return val;
}

static value_t *builtin_print(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        printf("\n");
        return value_null(vm);
    }

    value_t *arg = args->as.pair.car;

    if (value_is_null(arg)) {
        printf("()\n");
    } else if (value_is_bool(arg)) {
        printf("%s\n", arg->as.boolean ? "#t" : "#f");
    } else if (value_is_number(arg)) {
        if (arg->as.number == (double)arg->as.number) {
            printf("%llu\n", (unsigned long long)arg->as.number);
        } else {
            printf("%g\n", arg->as.floating);
        }
    } else if (value_is_string(arg)) {
        printf("%s\n", arg->as.string);
    } else if (value_is_symbol(arg)) {
        printf("%s\n", arg->as.symbol);
    } else {
        printf("#<value>\n");
    }

    return arg;
}

static value_t *builtin_shell(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "shell: expected 1 argument");
        return NULL;
    }
    value_t *cmd = args->as.pair.car;
    if (!value_is_string(cmd)) {
        vm_set_error(vm, VERR_TYPE, "shell: expected string");
        return NULL;
    }

    FILE *fp = popen(cmd->as.string, "r");
    if (!fp) {
        vm_set_error(vm, VERR_RUNTIME, "shell: failed to execute command");
        return NULL;
    }

    char buffer[4096];
    size_t total_size = 0;
    size_t capacity = 4096;
    char *output = malloc(capacity);
    if (!output) {
        pclose(fp);
        vm_set_error(vm, VERR_RUNTIME, "shell: memory allocation failed");
        return NULL;
    }

    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (total_size + n >= capacity) {
            capacity *= 2;
            char *new_output = realloc(output, capacity);
            if (!new_output) {
                free(output);
                pclose(fp);
                vm_set_error(vm, VERR_RUNTIME, "shell: memory allocation failed");
                return NULL;
            }
            output = new_output;
        }
        memcpy(output + total_size, buffer, n);
        total_size += n;
    }

    output[total_size] = '\0';
    pclose(fp);

    value_t *result = value_string(vm, output);
    free(output);
    return result;
}

static value_t *builtin_curl_json(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "curl-json: expected 1 argument");
        return NULL;
    }
    value_t *url = args->as.pair.car;
    if (!value_is_string(url)) {
        vm_set_error(vm, VERR_TYPE, "curl-json: expected string URL");
        return NULL;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "curl -s '%s'", url->as.string);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        vm_set_error(vm, VERR_RUNTIME, "curl-json: failed to execute curl");
        return NULL;
    }

    char buffer[4096];
    size_t total_size = 0;
    size_t capacity = 4096;
    char *output = malloc(capacity);
    if (!output) {
        pclose(fp);
        vm_set_error(vm, VERR_RUNTIME, "curl-json: memory allocation failed");
        return NULL;
    }

    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (total_size + n >= capacity) {
            capacity *= 2;
            char *new_output = realloc(output, capacity);
            if (!new_output) {
                free(output);
                pclose(fp);
                vm_set_error(vm, VERR_RUNTIME, "curl-json: memory allocation failed");
                return NULL;
            }
            output = new_output;
        }
        memcpy(output + total_size, buffer, n);
        total_size += n;
    }

    output[total_size] = '\0';
    pclose(fp);

    value_t *json_val = json_parse(vm, output);
    free(output);
    return json_val;
}

static value_t *builtin_json_parse(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "json-parse: expected 1 argument");
        return NULL;
    }
    value_t *str = args->as.pair.car;
    if (!value_is_string(str)) {
        vm_set_error(vm, VERR_TYPE, "json-parse: expected string");
        return NULL;
    }
    return json_parse(vm, str->as.string);
}

static value_t *builtin_json_stringify(vm_t *vm, value_t *args) {
    if (value_is_null(args)) {
        vm_set_error(vm, VERR_ARGS, "json-stringify: expected 1 argument");
        return NULL;
    }
    value_t *val = args->as.pair.car;
    return json_stringify(vm, val);
}

static value_t *builtin_json_select(vm_t *vm, value_t *args) {
    if (value_is_null(args) || value_is_null(args->as.pair.cdr)) {
        vm_set_error(vm, VERR_ARGS, "json-select: expected 2 arguments");
        return NULL;
    }
    value_t *obj = args->as.pair.car;
    value_t *path = args->as.pair.cdr->as.pair.car;
    return json_select(vm, obj, path);
}

static value_t *builtin_string_append(vm_t *vm, value_t *args) {
    size_t total_len = 0;
    value_t *arg = args;
    while (!value_is_null(arg)) {
        value_t *str = arg->as.pair.car;
        if (!value_is_string(str)) {
            vm_set_error(vm, VERR_TYPE, "string-append: expected strings");
            return NULL;
        }
        total_len += strlen(str->as.string);
        arg = arg->as.pair.cdr;
    }

    char *result = malloc(total_len + 1);
    if (!result) {
        vm_set_error(vm, VERR_RUNTIME, "string-append: memory allocation failed");
        return NULL;
    }

    result[0] = '\0';
    arg = args;
    while (!value_is_null(arg)) {
        value_t *str = arg->as.pair.car;
        strcat(result, str->as.string);
        arg = arg->as.pair.cdr;
    }

    value_t *val = value_string(vm, result);
    free(result);
    return val;
}

void vm_register_builtins(vm_t *vm) {
    vm_register_native(vm, "+", builtin_add);
    vm_register_native(vm, "-", builtin_sub);
    vm_register_native(vm, "*", builtin_mul);
    vm_register_native(vm, "/", builtin_div);
    vm_register_native(vm, "=", builtin_eq);
    vm_register_native(vm, "<", builtin_lt);
    vm_register_native(vm, ">", builtin_gt);
    vm_register_native(vm, "cons", builtin_cons);
    vm_register_native(vm, "car", builtin_car);
    vm_register_native(vm, "cdr", builtin_cdr);
    vm_register_native(vm, "list", builtin_list);
    vm_register_native(vm, "null?", builtin_null_p);
    vm_register_native(vm, "pair?", builtin_pair_p);
    vm_register_native(vm, "number?", builtin_number_p);
    vm_register_native(vm, "string?", builtin_string_p);
    vm_register_native(vm, "symbol?", builtin_symbol_p);
    vm_register_native(vm, "vector?", builtin_vector_p);
    vm_register_native(vm, "hash?", builtin_hash_p);
    vm_register_native(vm, "vector", builtin_vector);
    vm_register_native(vm, "hash", builtin_hash);
    vm_register_native(vm, "hash-set!", builtin_hash_set);
    vm_register_native(vm, "hash-ref", builtin_hash_ref);
    vm_register_native(vm, "vector-ref", builtin_vector_ref);
    vm_register_native(vm, "vector-set!", builtin_vector_set);
    vm_register_native(vm, "print", builtin_print);
    vm_register_native(vm, "shell", builtin_shell);
    vm_register_native(vm, "curl-json", builtin_curl_json);
    vm_register_native(vm, "json-parse", builtin_json_parse);
    vm_register_native(vm, "json-stringify", builtin_json_stringify);
    vm_register_native(vm, "json-select", builtin_json_select);
    vm_register_native(vm, "string-append", builtin_string_append);
}