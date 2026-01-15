#include "reader.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

reader_t *reader_create(vm_t *vm, const char *input) {
    reader_t *r = calloc(1, sizeof(reader_t));
    if (!r) return NULL;

    r->input = input;
    r->len = strlen(input);
    r->pos = 0;
    r->vm = vm;

    return r;
}

void reader_destroy(reader_t *r) {
    free(r);
}

int reader_skip_whitespace(reader_t *r) {
    while (r->pos < r->len) {
        char c = r->input[r->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            r->pos++;
        } else if (c == ';') {
            while (r->pos < r->len && r->input[r->pos] != '\n') {
                r->pos++;
            }
        } else {
            break;
        }
    }
    return r->pos < r->len;
}

int reader_peek(reader_t *r) {
    if (r->pos >= r->len) return -1;
    return r->input[r->pos];
}

int reader_next(reader_t *r) {
    if (r->pos >= r->len) return -1;
    return r->input[r->pos++];
}

int reader_match(reader_t *r, char c) {
    if (reader_peek(r) == c) {
        reader_next(r);
        return 1;
    }
    return 0;
}

value_t *read_string(reader_t *r) {
    reader_next(r);

    char buf[4096];
    size_t len = 0;

    while (r->pos < r->len) {
        int c = reader_next(r);
        if (c == '"') {
            buf[len] = '\0';
            return value_string(r->vm, buf);
        }
        if (c == '\\' && r->pos < r->len) {
            int next = reader_next(r);
            switch (next) {
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                default: c = next; break;
            }
        }
        if (len < sizeof(buf) - 1) {
            buf[len++] = c;
        }
    }

    vm_set_error(r->vm, VERR_SYNTAX, "unterminated string");
    return NULL;
}

value_t *read_number(reader_t *r) {
    char buf[256];
    size_t len = 0;
    int is_float = 0;

    int c = reader_peek(r);
    if (c == '-') {
        buf[len++] = reader_next(r);
    }

    while (r->pos < r->len && isdigit(r->input[r->pos]) && len < sizeof(buf) - 1) {
        buf[len++] = reader_next(r);
    }

    if (r->pos < r->len && r->input[r->pos] == '.') {
        is_float = 1;
        buf[len++] = reader_next(r);
        while (r->pos < r->len && isdigit(r->input[r->pos]) && len < sizeof(buf) - 1) {
            buf[len++] = reader_next(r);
        }
    }

    buf[len] = '\0';

    if (is_float) {
        double d = strtod(buf, NULL);
        return value_double(r->vm, d);
    } else {
        uint64_t n = strtoull(buf, NULL, 10);
        return value_number(r->vm, n);
    }
}

value_t *read_symbol(reader_t *r) {
    char buf[256];
    size_t len = 0;

    while (r->pos < r->len) {
        int c = r->input[r->pos];
        if (c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
            c == '"' || c == ';' || c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            break;
        }
        if (len < sizeof(buf) - 1) {
            buf[len++] = c;
        }
        r->pos++;
    }

    buf[len] = '\0';

    if (strcmp(buf, "null") == 0 || strcmp(buf, "#f") == 0 || strcmp(buf, "#t") == 0) {
        if (strcmp(buf, "#t") == 0 || strcmp(buf, "true") == 0) {
            return value_bool(r->vm, 1);
        }
        if (strcmp(buf, "#f") == 0 || strcmp(buf, "false") == 0) {
            return value_bool(r->vm, 0);
        }
        return value_null(r->vm);
    }

    return value_symbol(r->vm, buf);
}

value_t *read_list(reader_t *r, char end) {
    value_t *head = value_null(r->vm);
    value_t **tail = &head;

    while (reader_skip_whitespace(r)) {
        if (reader_match(r, end)) {
            return head;
        }

        value_t *item = reader_read(r);
        if (!item) {
            value_release(r->vm, head);
            return NULL;
        }

        if (reader_peek(r) == '.') {
            reader_next(r);
            reader_skip_whitespace(r);
            value_t *rest = reader_read(r);
            if (!rest) {
                value_release(r->vm, head);
                return NULL;
            }
            *tail = rest;
            reader_skip_whitespace(r);
            if (!reader_match(r, end)) {
                vm_set_error(r->vm, VERR_SYNTAX, "expected %c", end);
                value_release(r->vm, head);
                return NULL;
            }
            return head;
        }

        *tail = value_pair(r->vm, item, value_null(r->vm));
        tail = &((*tail)->as.pair.cdr);
    }

    vm_set_error(r->vm, VERR_SYNTAX, "unterminated list");
    value_release(r->vm, head);
    return NULL;
}

value_t *read_vector(reader_t *r) {
    reader_next(r);

    value_t *vec = value_vector(r->vm);
    if (!vec) return NULL;

    reader_skip_whitespace(r);
    while (reader_peek(r) != ']') {
        value_t *item = reader_read(r);
        if (!item) {
            value_release(r->vm, vec);
            return NULL;
        }
        vector_push(r->vm, vec, item);
        reader_skip_whitespace(r);
    }

    reader_next(r);
    return vec;
}

value_t *read_hash(reader_t *r) {
    reader_next(r);

    value_t *hash = value_hash(r->vm);
    if (!hash) return NULL;

    reader_skip_whitespace(r);
    while (reader_peek(r) != '}') {
        value_t *key = reader_read(r);
        if (!key) {
            value_release(r->vm, hash);
            return NULL;
        }

        reader_skip_whitespace(r);

        value_t *val = reader_read(r);
        if (!val) {
            value_release(r->vm, key);
            value_release(r->vm, hash);
            return NULL;
        }

        hash_set(r->vm, hash, key, val);

        reader_skip_whitespace(r);
    }

    reader_next(r);
    return hash;
}

value_t *reader_read(reader_t *r) {
    if (!reader_skip_whitespace(r)) return NULL;

    int c = reader_peek(r);

    if (c == '(') {
        reader_next(r);
        return read_list(r, ')');
    }

    if (c == '[') {
        return read_vector(r);
    }

    if (c == '{') {
        return read_hash(r);
    }

    if (c == '\'') {
        reader_next(r);
        value_t *quoted = reader_read(r);
        if (!quoted) return NULL;
        value_t *quote_sym = value_symbol(r->vm, "quote");
        return value_pair(r->vm, quote_sym, value_pair(r->vm, quoted, value_null(r->vm)));
    }

    if (c == '"') {
        return read_string(r);
    }

    if (c == '-' || isdigit(c)) {
        return read_number(r);
    }

    return read_symbol(r);
}