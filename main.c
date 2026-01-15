#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"

void vm_register_builtins(vm_t *vm);

int main(int argc, char *argv[]) {
    vm_t *vm = scheme_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    // If arguments provided, execute scripts and exit
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            FILE *f = fopen(argv[i], "r");
            if (!f) {
                fprintf(stderr, "Failed to open file: %s\n", argv[i]);
                scheme_destroy(vm);
                return 1;
            }

            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);

            char *code = malloc(size + 1);
            if (!code) {
                fprintf(stderr, "Failed to allocate memory\n");
                fclose(f);
                scheme_destroy(vm);
                return 1;
            }

            if (fread(code, 1, size, f) != (size_t)size) {
                fprintf(stderr, "Failed to read file: %s\n", argv[i]);
                free(code);
                fclose(f);
                scheme_destroy(vm);
                return 1;
            }
            code[size] = '\0';
            fclose(f);

            value_t *result;
            int ret = scheme_eval_string(vm, code, &result);
            free(code);

            if (ret == 0) {
                fprintf(stderr, "Error in %s: %s\n", argv[i], scheme_error_message(vm));
                scheme_clear_error(vm);
                scheme_destroy(vm);
                return 1;
            }
        }

        scheme_destroy(vm);
        return 0;
    }

    // Otherwise, run REPL
    char *line = NULL;
    size_t len = 0;
    ssize_t n;

    printf("pscm REPL\n");
    printf("Type 'quit' to exit\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        n = getline(&line, &len, stdin);
        if (n == -1) {
            break;
        }

        // Remove trailing newline
        if (n > 0 && line[n-1] == '\n') {
            line[n-1] = '\0';
        }

        // Check for quit
        if (strcmp(line, "quit") == 0) {
            break;
        }

        value_t *result;
        int ret = scheme_eval_string(vm, line, &result);
        if (ret == 0) {
            // Print result
            char buf[4096];
            if (scheme_to_string_copy(result, buf, sizeof(buf))) {
                printf("%s\n", buf);
            } else {
                printf("#<value>\n");
            }
        } else {
            printf("Error: %s\n", scheme_error_message(vm));
            scheme_clear_error(vm);
        }
    }

    free(line);
    scheme_destroy(vm);
    return 0;
}