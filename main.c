#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"

void vm_register_builtins(vm_t *vm);

int main() {
    vm_t *vm = scheme_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    vm_register_builtins(vm);

    char *line = NULL;
    size_t len = 0;
    ssize_t n;

    printf("vibescheme REPL\n");
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