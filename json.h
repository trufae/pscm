#ifndef JSON_H
#define JSON_H

#include "vm.h"
#include "value.h"

value_t *json_parse(vm_t *vm, const char *json_str);
value_t *json_stringify(vm_t *vm, value_t *val);
value_t *json_select(vm_t *vm, value_t *obj, value_t *path);

int json_write_string(char *buf, size_t *pos, size_t len, const char *str);
int json_write_value(char *buf, size_t *pos, size_t len, value_t *val);

#endif