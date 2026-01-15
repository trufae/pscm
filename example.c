#include "vibescheme.h"
#include <stdio.h>

int main() {
    vm_t *vm = scheme_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    // Test basic arithmetic
    value_t *result;
    if (scheme_eval_string(vm, "(+ 1 2 3)", &result)) {
        uint64_t num;
        if (scheme_to_number(result, &num)) {
            printf("(+ 1 2 3) = %llu\n", (unsigned long long)num);
        }
        scheme_release(vm, result);
    } else {
        fprintf(stderr, "Error: %s\n", scheme_error_message(vm));
    }

    // Test JSON parsing
    value_t *json_val;
    if (scheme_eval_string(vm, "(json-parse \"{\\\"name\\\":\\\"test\\\",\\\"value\\\":42}\")", &json_val)) {
        if (scheme_is_hash(json_val)) {
            value_t *name_key = scheme_make_string(vm, "name");
            value_t *name_val = scheme_hash_get(vm, json_val, name_key);
            if (name_val && scheme_is_string(name_val)) {
                const char *name_str;
                if (scheme_to_string(name_val, &name_str)) {
                    printf("JSON name: %s\n", name_str);
                }
            }
            scheme_release(vm, name_key);
        }
        scheme_release(vm, json_val);
    } else {
        fprintf(stderr, "JSON Error: %s\n", scheme_error_message(vm));
    }

    // Test callable hash/vector (Lispy JSON access)
    if (scheme_eval_string(vm, "(define data (json-parse \"[{\\\"name\\\":\\\"item1\\\",\\\"val\\\":10}]\"))", NULL)) {
        if (scheme_eval_string(vm, "((data 0) \"val\")", &result)) {
            uint64_t num;
            if (scheme_to_number(result, &num)) {
                printf("(data 0 \"val\") = %llu\n", (unsigned long long)num);
            }
            scheme_release(vm, result);
        }
    }

    scheme_destroy(vm);
    return 0;
}