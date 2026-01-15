#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct json_parser {
    const char *input;
    size_t pos;
    size_t len;
    vm_t *vm;
} json_parser_t;

static value_t *json_parse_value(json_parser_t *p);

static void json_skip_whitespace(json_parser_t *p) {
    while (p->pos < p->len) {
        char c = p->input[p->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            p->pos++;
        } else {
            break;
        }
    }
}

static char json_peek(json_parser_t *p) {
    if (p->pos >= p->len) return '\0';
    return p->input[p->pos];
}

static char json_next(json_parser_t *p) {
    if (p->pos >= p->len) return '\0';
    return p->input[p->pos++];
}

static value_t *json_parse_string(json_parser_t *p) {
    json_next(p);

    char buf[4096];
    size_t len = 0;

    while (p->pos < p->len) {
        char c = json_next(p);
        if (c == '"') {
            buf[len] = '\0';
            return value_string(p->vm, buf);
        }
        if (c == '\\' && p->pos < p->len) {
            char next = json_next(p);
            switch (next) {
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                case '/': c = '/'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'u': {
                    if (p->pos + 4 >= p->len) break;
                    char hex[5];
                    memcpy(hex, &p->input[p->pos], 4);
                    hex[4] = '\0';
                    p->pos += 4;
                    unsigned int codepoint = strtoul(hex, NULL, 16);
                    if (codepoint < 0x80) {
                        c = codepoint;
                    } else if (codepoint < 0x800) {
                        if (len + 1 < sizeof(buf)) {
                            buf[len++] = (char)(0xC0 | (codepoint >> 6));
                            buf[len++] = (char)(0x80 | (codepoint & 0x3F));
                        }
                        continue;
                    } else {
                        if (len + 2 < sizeof(buf)) {
                            buf[len++] = (char)(0xE0 | (codepoint >> 12));
                            buf[len++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                            buf[len++] = (char)(0x80 | (codepoint & 0x3F));
                        }
                        continue;
                    }
                    break;
                }
                default: c = next; break;
            }
        }
        if (len < sizeof(buf) - 1) {
            buf[len++] = c;
        }
    }

    vm_set_error(p->vm, VERR_RUNTIME, "unterminated JSON string");
    return NULL;
}

static value_t *json_parse_number(json_parser_t *p) {
    char buf[64];
    size_t len = 0;

    if (json_peek(p) == '-') {
        buf[len++] = json_next(p);
    }

    while (p->pos < p->len && isdigit(json_peek(p)) && len < sizeof(buf) - 1) {
        buf[len++] = json_next(p);
    }

    if (json_peek(p) == '.') {
        buf[len++] = json_next(p);
        while (p->pos < p->len && isdigit(json_peek(p)) && len < sizeof(buf) - 1) {
            buf[len++] = json_next(p);
        }
    }

    if (json_peek(p) == 'e' || json_peek(p) == 'E') {
        buf[len++] = json_next(p);
        if (json_peek(p) == '+' || json_peek(p) == '-') {
            buf[len++] = json_next(p);
        }
        while (p->pos < p->len && isdigit(json_peek(p)) && len < sizeof(buf) - 1) {
            buf[len++] = json_next(p);
        }
    }

    buf[len] = '\0';

    if (strchr(buf, '.') || strchr(buf, 'e') || strchr(buf, 'E')) {
        double d = strtod(buf, NULL);
        return value_double(p->vm, d);
    } else {
        uint64_t n = strtoull(buf, NULL, 10);
        return value_number(p->vm, n);
    }
}

static value_t *json_parse_array(json_parser_t *p) {
    json_next(p);

    value_t *vec = value_vector(p->vm);
    if (!vec) return NULL;

    json_skip_whitespace(p);

    if (json_peek(p) == ']') {
        json_next(p);
        return vec;
    }

    while (1) {
        json_skip_whitespace(p);

        value_t *elem = json_parse_value(p);
        if (!elem) {
            value_release(p->vm, vec);
            return NULL;
        }

        vector_push(p->vm, vec, elem);

        json_skip_whitespace(p);

        char c = json_peek(p);
        if (c == ']') {
            json_next(p);
            return vec;
        }
        if (c == ',') {
            json_next(p);
            continue;
        }

        vm_set_error(p->vm, VERR_RUNTIME, "expected ',' or ']' in JSON array");
        value_release(p->vm, vec);
        return NULL;
    }
}

static value_t *json_parse_object(json_parser_t *p) {
    json_next(p);

    value_t *hash = value_hash(p->vm);
    if (!hash) return NULL;

    json_skip_whitespace(p);

    if (json_peek(p) == '}') {
        json_next(p);
        return hash;
    }

    while (1) {
        json_skip_whitespace(p);

        if (json_peek(p) != '"') {
            vm_set_error(p->vm, VERR_RUNTIME, "expected string key in JSON object");
            value_release(p->vm, hash);
            return NULL;
        }

        value_t *key = json_parse_string(p);
        if (!key) {
            value_release(p->vm, hash);
            return NULL;
        }

        json_skip_whitespace(p);

        if (json_peek(p) != ':') {
            vm_set_error(p->vm, VERR_RUNTIME, "expected ':' in JSON object");
            value_release(p->vm, key);
            value_release(p->vm, hash);
            return NULL;
        }
        json_next(p);

        json_skip_whitespace(p);

        value_t *val = json_parse_value(p);
        if (!val) {
            value_release(p->vm, key);
            value_release(p->vm, hash);
            return NULL;
        }

        hash_set(p->vm, hash, key, val);

        json_skip_whitespace(p);

        char c = json_peek(p);
        if (c == '}') {
            json_next(p);
            return hash;
        }
        if (c == ',') {
            json_next(p);
            continue;
        }

        vm_set_error(p->vm, VERR_RUNTIME, "expected ',' or '}' in JSON object");
        value_release(p->vm, hash);
        return NULL;
    }
}

static value_t *json_parse_value(json_parser_t *p) {
    json_skip_whitespace(p);
    char c = json_peek(p);

    if (c == '"') return json_parse_string(p);
    if (c == '{') return json_parse_object(p);
    if (c == '[') return json_parse_array(p);
    if (c == '-' || isdigit(c)) return json_parse_number(p);

    if (c == 't') {
        if (p->pos + 3 <= p->len && strncmp(&p->input[p->pos], "true", 4) == 0) {
            p->pos += 4;
            return value_bool(p->vm, 1);
        }
    }

    if (c == 'f') {
        if (p->pos + 4 <= p->len && strncmp(&p->input[p->pos], "false", 5) == 0) {
            p->pos += 5;
            return value_bool(p->vm, 0);
        }
    }

    if (c == 'n') {
        if (p->pos + 3 <= p->len && strncmp(&p->input[p->pos], "null", 4) == 0) {
            p->pos += 4;
            return value_null(p->vm);
        }
    }

    vm_set_error(p->vm, VERR_RUNTIME, "invalid JSON");
    return NULL;
}

value_t *json_parse(vm_t *vm, const char *json_str) {
    json_parser_t p = { json_str, 0, strlen(json_str), vm };
    value_t *result = json_parse_value(&p);

    json_skip_whitespace(&p);
    if (p.pos < p.len && result) {
        vm_set_error(vm, VERR_RUNTIME, "trailing data in JSON");
        value_release(vm, result);
        return NULL;
    }

    return result;
}

int json_write_string(char *buf, size_t *pos, size_t len, const char *str) {
    if (*pos >= len) return 0;

    if (*pos < len) buf[(*pos)++] = '"';

    for (const char *s = str; *s && *pos < len; s++) {
        char c = *s;
        switch (c) {
            case '"':
            case '\\':
            case '/':
                if (*pos + 1 < len) {
                    buf[(*pos)++] = '\\';
                    buf[(*pos)++] = c;
                }
                break;
            case '\b':
                if (*pos + 1 < len) {
                    buf[(*pos)++] = '\\';
                    buf[(*pos)++] = 'b';
                }
                break;
            case '\f':
                if (*pos + 1 < len) {
                    buf[(*pos)++] = '\\';
                    buf[(*pos)++] = 'f';
                }
                break;
            case '\n':
                if (*pos + 1 < len) {
                    buf[(*pos)++] = '\\';
                    buf[(*pos)++] = 'n';
                }
                break;
            case '\r':
                if (*pos + 1 < len) {
                    buf[(*pos)++] = '\\';
                    buf[(*pos)++] = 'r';
                }
                break;
            case '\t':
                if (*pos + 1 < len) {
                    buf[(*pos)++] = '\\';
                    buf[(*pos)++] = 't';
                }
                break;
            default:
                if (c < ' ') {
                    char hex[8];
                    snprintf(hex, sizeof(hex), "\\u%04x", (unsigned char)c);
                    for (size_t i = 0; hex[i] && *pos < len; i++) {
                        buf[(*pos)++] = hex[i];
                    }
                } else {
                    buf[(*pos)++] = c;
                }
                break;
        }
    }

    if (*pos < len) buf[(*pos)++] = '"';
    return *pos < len;
}

int json_write_value(char *buf, size_t *pos, size_t len, value_t *val) {
    if (!val) {
        if (*pos + 4 <= len) {
            memcpy(&buf[*pos], "null", 4);
            *pos += 4;
        }
        return *pos <= len;
    }

    switch (val->type) {
        case VTYPE_NULL:
            if (*pos + 4 <= len) {
                memcpy(&buf[*pos], "null", 4);
                *pos += 4;
            }
            break;
        case VTYPE_BOOL:
            if (val->as.boolean) {
                if (*pos + 4 <= len) {
                    memcpy(&buf[*pos], "true", 4);
                    *pos += 4;
                }
            } else {
                if (*pos + 5 <= len) {
                    memcpy(&buf[*pos], "false", 5);
                    *pos += 5;
                }
            }
            break;
        case VTYPE_NUMBER:
            if (val->as.number == (double)val->as.number) {
                char num[32];
                int n = snprintf(num, sizeof(num), "%llu", (unsigned long long)val->as.number);
                if (*pos + n <= len) {
                    memcpy(&buf[*pos], num, n);
                    *pos += n;
                }
            } else {
                char num[64];
                int n = snprintf(num, sizeof(num), "%g", val->as.floating);
                if (*pos + n <= len) {
                    memcpy(&buf[*pos], num, n);
                    *pos += n;
                }
            }
            break;
        case VTYPE_STRING:
            json_write_string(buf, pos, len, val->as.string);
            break;
        case VTYPE_VECTOR: {
            if (*pos >= len) return 0;
            buf[(*pos)++] = '[';

            for (size_t i = 0; i < val->as.vector.size; i++) {
                if (i > 0) {
                    if (*pos >= len) return 0;
                    buf[(*pos)++] = ',';
                }
                json_write_value(buf, pos, len, val->as.vector.elements[i]);
            }

            if (*pos >= len) return 0;
            buf[(*pos)++] = ']';
            break;
        }
        case VTYPE_HASH: {
            if (*pos >= len) return 0;
            buf[(*pos)++] = '{';

            int first = 1;
            for (size_t i = 0; i < val->as.hash.size; i++) {
                if (val->as.hash.keys[i]) {
                    if (!first) {
                        if (*pos >= len) return 0;
                        buf[(*pos)++] = ',';
                    }
                    first = 0;

                    if (value_is_string(val->as.hash.keys[i])) {
                        json_write_string(buf, pos, len, val->as.hash.keys[i]->as.string);
                    } else if (value_is_number(val->as.hash.keys[i])) {
                        json_write_value(buf, pos, len, val->as.hash.keys[i]);
                    } else {
                        continue;
                    }

                    if (*pos >= len) return 0;
                    buf[(*pos)++] = ':';

                    json_write_value(buf, pos, len, val->as.hash.values[i]);
                }
            }

            if (*pos >= len) return 0;
            buf[(*pos)++] = '}';
            break;
        }
        default:
            if (*pos + 4 <= len) {
                memcpy(&buf[*pos], "null", 4);
                *pos += 4;
            }
            break;
    }

    return *pos <= len;
}

value_t *json_stringify(vm_t *vm, value_t *val) {
    char buf[65536];
    size_t pos = 0;

    if (!json_write_value(buf, &pos, sizeof(buf), val)) {
        vm_set_error(vm, VERR_RUNTIME, "JSON stringification failed: buffer too small");
        return NULL;
    }

    return value_string(vm, buf);
}

value_t *json_select(vm_t *vm, value_t *obj, value_t *path) {
    value_t *current = obj;

    value_t *item = path;
    while (!value_is_null(item)) {
        if (!current) {
            vm_set_error(vm, VERR_RUNTIME, "json-select: null path element");
            return NULL;
        }

        value_t *key = item->as.pair.car;

        if (value_is_hash(current)) {
            if (!value_is_string(key) && !value_is_number(key)) {
                vm_set_error(vm, VERR_TYPE, "json-select: hash keys must be strings or numbers");
                return NULL;
            }
            current = hash_get(vm, current, key);
        } else if (value_is_vector(current)) {
            if (!value_is_number(key)) {
                vm_set_error(vm, VERR_TYPE, "json-select: vector index must be number");
                return NULL;
            }
            current = vector_get(vm, current, (size_t)key->as.number);
        } else {
            vm_set_error(vm, VERR_TYPE, "json-select: expected hash or vector");
            return NULL;
        }

        item = item->as.pair.cdr;
    }

    return current ? current : value_null(vm);
}