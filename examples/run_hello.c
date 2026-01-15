#include "pscm.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    vm_t *vm = scheme_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    FILE *f = fopen("examples/hello.pscm", "r");
    if (!f) {
        fprintf(stderr, "Failed to open examples/hello.pscm\n");
        scheme_destroy(vm);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *code = malloc(len + 1);
    if (!code) {
        fprintf(stderr, "Failed to allocate memory\n");
        fclose(f);
        scheme_destroy(vm);
        return 1;
    }

    fread(code, 1, len, f);
    code[len] = '\0';
    fclose(f);

    value_t *result;
    if (scheme_eval_string(vm, code, &result)) {
        // Success, result is the last value
        printf("Evaluation successful\n");
        scheme_release(vm, result);
    } else {
        fprintf(stderr, "Error: %s\n", scheme_error_message(vm));
        scheme_clear_error(vm);
    }

    free(code);
    scheme_destroy(vm);
    return 0;
}