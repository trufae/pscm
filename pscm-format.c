#include "reader.h"
#include "value.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

typedef struct {
    int use_spaces;
    int indent_count;
    int current_indent;
} formatter_t;

static int is_null(value_t *v) { return !v || v->type == VTYPE_NULL; }

static void print_indent(formatter_t *fmt) {
    if (fmt->use_spaces) {
        for (int i = 0; i < fmt->current_indent; i++) {
            putchar(' ');
        }
    } else {
        int tabs = fmt->current_indent / fmt->indent_count;
        for (int i = 0; i < tabs; i++) {
            putchar('\t');
        }
    }
}

static int should_fit_on_one_line(value_t *v) {
    if (!v || v->type != VTYPE_PAIR) return 0;
    
    int count = 0;
    value_t *curr = v;
    while (curr && curr->type == VTYPE_PAIR) {
        count++;
        if (count > 4) return 0;
        
        value_t *car = curr->as.pair.car;
        if (car && car->type == VTYPE_PAIR) return 0;
        
        curr = curr->as.pair.cdr;
    }
    
    return 1;
}

static void print_value(value_t *v, formatter_t *fmt) {
    if (!v) {
        return;
    }

    switch (v->type) {
        case VTYPE_NULL:
            fputs("null", stdout);
            break;
        case VTYPE_BOOL:
            fputs(v->as.boolean ? "#t" : "#f", stdout);
            break;
        case VTYPE_NUMBER:
            printf("%llu", (unsigned long long)v->as.number);
            break;
        case VTYPE_STRING:
            putchar('"');
            for (char *s = v->as.string; *s; s++) {
                switch (*s) {
                    case '\n': fputs("\\n", stdout); break;
                    case '\r': fputs("\\r", stdout); break;
                    case '\t': fputs("\\t", stdout); break;
                    case '\\': fputs("\\\\", stdout); break;
                    case '"': fputs("\\\"", stdout); break;
                    default: putchar(*s); break;
                }
            }
            putchar('"');
            break;
        case VTYPE_SYMBOL:
            fputs(v->as.symbol, stdout);
            break;
        case VTYPE_PAIR: {
            int fits = should_fit_on_one_line(v);
            
            putchar('(');
            
            if (fits) {
                value_t *curr = v;
                int first = 1;
                while (curr && curr->type == VTYPE_PAIR) {
                    if (!first) putchar(' ');
                    first = 0;
                    print_value(curr->as.pair.car, fmt);
                    curr = curr->as.pair.cdr;
                }
                if (curr && !is_null(curr)) {
                    if (!first) putchar(' ');
                    fputs(" . ", stdout);
                    print_value(curr, fmt);
                }
            } else {
                fmt->current_indent += fmt->indent_count;
                
                int first = 1;
                value_t *curr = v;
                while (curr && curr->type == VTYPE_PAIR) {
                    if (!first) {
                        putchar('\n');
                        print_indent(fmt);
                    }
                    first = 0;
                    print_value(curr->as.pair.car, fmt);
                    curr = curr->as.pair.cdr;
                }
                
                if (curr && !is_null(curr)) {
                    putchar('\n');
                    print_indent(fmt);
                    fputs(". ", stdout);
                    print_value(curr, fmt);
                }
                
                fmt->current_indent -= fmt->indent_count;
            }
            
            putchar(')');
            break;
        }
        case VTYPE_VECTOR: {
            putchar('[');
            fmt->current_indent += fmt->indent_count;
            
            for (size_t i = 0; i < v->as.vector.size; i++) {
                if (i > 0) {
                    putchar('\n');
                    print_indent(fmt);
                }
                print_value(v->as.vector.elements[i], fmt);
            }
            
            fmt->current_indent -= fmt->indent_count;
            putchar(']');
            break;
        }
        case VTYPE_HASH: {
            putchar('{');
            fmt->current_indent += fmt->indent_count;
            
            int first = 1;
            for (size_t i = 0; i < v->as.hash.size; i++) {
                if (!first) {
                    putchar('\n');
                    print_indent(fmt);
                }
                first = 0;
                print_value(v->as.hash.keys[i], fmt);
                putchar(' ');
                print_value(v->as.hash.values[i], fmt);
            }
            
            fmt->current_indent -= fmt->indent_count;
            putchar('}');
            break;
        }
        case VTYPE_LAMBDA:
            fputs("#<lambda>", stdout);
            break;
        case VTYPE_NATIVE:
            fputs("#<native>", stdout);
            break;
    }
}

static char *read_file_contents(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Failed to open file");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);

    return buf;
}

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [OPTIONS] [FILE]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s, --spaces N    Use N spaces per indent instead of tabs\n");
    fprintf(stderr, "  -h, --help       Show this help message\n");
}

int main(int argc, char *argv[]) {
    formatter_t fmt = {
        .use_spaces = 0,
        .indent_count = 1,
        .current_indent = 0,
    };

    static struct option long_options[] = {
        {"spaces", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "s:h", long_options, NULL)) != -1) {
        switch (c) {
            case 's':
                fmt.use_spaces = 1;
                fmt.indent_count = atoi(optarg);
                if (fmt.indent_count <= 0) {
                    fprintf(stderr, "Error: --spaces must be positive\n");
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    const char *filename = NULL;
    if (optind < argc) {
        filename = argv[optind];
    }

    char *input = NULL;
    if (filename) {
        input = read_file_contents(filename);
        if (!input) return 1;
    } else {
        FILE *f = fopen("/dev/stdin", "r");
        if (!f) {
            perror("Failed to open stdin");
            return 1;
        }

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        input = malloc(size + 1);
        if (!input) {
            fclose(f);
            return 1;
        }

        fread(input, 1, size, f);
        input[size] = '\0';
        fclose(f);
    }

    vm_t *vm = vm_create();
    if (!vm) {
        free(input);
        return 1;
    }

    reader_t *reader = reader_create(vm, input);
    if (!reader) {
        vm_destroy(vm);
        free(input);
        return 1;
    }

    int first = 1;
    while (1) {
        value_t *v = reader_read(reader);
        if (!v) {
            if (reader_peek(reader) == -1) break;
            
            const char *err = vm_error_message(vm);
            if (err) {
                fprintf(stderr, "Error: %s\n", err);
            } else {
                fprintf(stderr, "Parse error\n");
            }
            break;
        }

        if (!first && !is_null(v)) {
            putchar('\n');
            putchar('\n');
        }
        first = 0;

        print_value(v, &fmt);
    }

    reader_destroy(reader);
    vm_destroy(vm);
    free(input);

    putchar('\n');
    return 0;
}