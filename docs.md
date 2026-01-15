# Vibescheme Documentation

Vibescheme is a minimalistic Scheme interpreter designed to be embedded in C applications. It provides a clean, safe, and extensible platform for scripting with a focus on simplicity and performance.

## Overview

Vibescheme implements a subset of Scheme (Lisp dialect) with the following design goals:
- **KISS (Keep It Simple, Stupid)**: Minimal, clean codebase that's easy to understand and modify
- **Memory Safe**: Reference counting prevents memory leaks and dangling pointers
- **Interruptible**: C code can interrupt execution at any time for safety
- **JSON Native**: Seamless integration with JSON data structures
- **C Extensible**: Easy to register C functions and call Scheme from C

## Language Features

Vibescheme supports a core subset of Scheme:

### Data Types
- **Numbers**: uint64_t integers by default, double for floating point
- **Strings**: Immutable strings
- **Symbols**: Interned identifiers
- **Booleans**: #t and #f
- **Null**: ()
- **Pairs**: (cons a b)
- **Lists**: Built from pairs
- **Vectors**: [item1 item2 ...]
- **Hashes**: {key1 val1 key2 val2 ...}
- **Lambdas**: (lambda (params) body)
- **Native Functions**: C functions callable from Scheme

### Special Forms
- `(quote expr)` or `'expr`: Returns expr unevaluated
- `(define name value)`: Binds name to value in global environment
- `(lambda (params...) body...)`: Creates a function
- `(if test then else)`: Conditional evaluation
- `(let ((name val)...) body...)`: Local bindings

### Built-in Functions
- **Math**: `+`, `-`, `*`, `/`, `=`, `<`, `>`
- **Lists**: `cons`, `car`, `cdr`, `list`
- **Predicates**: `null?`, `pair?`, `number?`, `string?`, `symbol?`, `vector?`, `hash?`
- **Vectors**: `vector`, `vector-ref`, `vector-set!`
- **Hashes**: `hash`, `hash-ref`, `hash-set!`
- **JSON**: `json-parse`, `json-stringify`, `json-select`

### JSON Integration
JSON objects become Scheme hashes, arrays become vectors. Both are callable:
```scheme
(define data (json-parse "{\"users\":[{\"name\":\"Alice\",\"age\":30}]}"))
(define alice-age ((data "users") 0 "age"))  ; 30
```

## C API Usage

### Basic Setup
```c
#include "vibescheme.h"

int main() {
    vm_t *vm = scheme_create();
    if (!vm) return 1;

    // Use VM...

    scheme_destroy(vm);
    return 0;
}
```

### Evaluating Scheme Code
```c
value_t *result;
if (scheme_eval_string(vm, "(+ 1 2 3)", &result)) {
    uint64_t num;
    if (scheme_to_number(result, &num)) {
        printf("Result: %llu\n", (unsigned long long)num);
    }
    scheme_release(vm, result);
} else {
    printf("Error: %s\n", scheme_error_message(vm));
}
```

### Registering C Functions
```c
value_t *my_print(vm_t *vm, value_t *args) {
    value_t *str = args->as.pair.car;
    const char *s;
    if (scheme_to_string(str, &s)) {
        printf("Scheme says: %s\n", s);
    }
    return scheme_make_null(vm);
}

scheme_register_native(vm, "print", my_print);
// Now (print "Hello") works in Scheme
```

### Calling Scheme Functions from C
```c
value_t *args[2];
args[0] = scheme_make_number(vm, 10);
args[1] = scheme_make_number(vm, 20);

value_t *result = scheme_call(vm, "+", args, 2);
uint64_t sum;
scheme_to_number(result, &sum);
scheme_release(vm, result);
```

### JSON Handling
```c
value_t *json_data;
if (scheme_json_parse(vm, "{\"key\": \"value\"}", &json_data)) {
    value_t *key = scheme_make_string(vm, "key");
    value_t *val = scheme_hash_get(vm, json_data, key);
    // Use val...
    scheme_release(vm, json_data);
    scheme_release(vm, key);
}

char json_str[1024];
scheme_json_stringify(vm, json_data, json_str, sizeof(json_str));
```

### Error Handling
```c
scheme_clear_error(vm);
if (!scheme_eval_string(vm, code, &result)) {
    const char *err = scheme_error_message(vm);
    // Handle error
}
```

### Interrupting Execution
```c
// In signal handler or other thread
scheme_interrupt(vm);  // Stops execution safely
```

## Scheme Examples

### Basic Arithmetic
```scheme
(define x 42)
(define y (+ x 10))  ; 52
```

### Functions
```scheme
(define square (lambda (x) (* x x)))
(define result (square 5))  ; 25
```

### Lists
```scheme
(define my-list (list 1 2 3 4))
(define first (car my-list))      ; 1
(define rest (cdr my-list))       ; (2 3 4)
(define new-list (cons 0 my-list)) ; (0 1 2 3 4)
```

### Vectors
```scheme
(define vec [10 20 30])
(define second (vector-ref vec 1))     ; 20
(vector-set! vec 1 25)                  ; vec is now [10 25 30]
```

### Hashes
```scheme
(define h { "name" "Alice" "age" 30 })
(define name (hash-ref h "name"))       ; "Alice"
(hash-set! h "city" "NYC")               ; Adds to hash
```

### JSON Processing
```scheme
(define api-data (json-parse "{\"users\": [{\"id\": 1, \"name\": \"Bob\"}]}"))
(define user-name (((api-data "users") 0) "name"))  ; "Bob"
(define json-str (json-stringify api-data))          ; Back to JSON string
```

### Local Variables
```scheme
(define (factorial n)
  (if (= n 0)
      1
      (let ((prev (factorial (- n 1))))
        (* n prev))))
```

## Design Decisions

### Reference Counting
- **Why?** Prevents memory leaks and use-after-free bugs without complex GC
- **How?** Every value has a refcount; C code must retain/release references
- **Trade-off**: Manual memory management, but safe and predictable

### No Garbage Collector
- **Why?** Simplicity and performance; avoids GC pauses
- **How?** Reference counting with cycle detection via careful coding
- **Trade-off**: Potential reference cycles, but minimized in embedded use

### Interrupt Support
- **Why?** Allows safe termination of runaway scripts (e.g., infinite loops)
- **How?** VM checks interrupt flag before each expression evaluation
- **Use**: Signal handlers or timeouts can call `scheme_interrupt()`

### Numbers as uint64_t
- **Why?** Addresses and big integers are common in embedded systems
- **How?** Default numbers are uint64_t; doubles for floating point
- **Trade-off**: No automatic int/float conversion; explicit conversion needed

### Callable Collections
- **Why?** Natural JSON path access like `((obj "key") 0 "sub")`
- **How?** Vectors and hashes implement callable interface
- **Trade-off**: Slight evaluation overhead, but intuitive API

### Single-Threaded
- **Why?** Simplicity; most embedded use is single-threaded
- **How?** No locks or thread-safety mechanisms
- **Trade-off**: Not thread-safe; use external synchronization if needed

## Features Implemented

- [x] Core Scheme evaluation (define, lambda, if, let)
- [x] Basic data types (numbers, strings, symbols, pairs, vectors, hashes)
- [x] Arithmetic and comparison operators
- [x] List manipulation functions
- [x] Vector and hash operations
- [x] JSON parsing and stringification
- [x] Callable JSON access syntax
- [x] C API for embedding
- [x] Native function registration
- [x] Scheme function calling from C
- [x] Error handling and reporting
- [x] Execution interruption
- [x] Reference counting memory management
- [x] Makefile and build system

## How to Extend It

### Adding Built-in Functions
1. Implement C function: `value_t *my_func(vm_t *vm, value_t *args)`
2. Add to `builtin.c`: Register in `vm_register_builtins()`
3. Or register dynamically: `scheme_register_native(vm, "my-func", my_func)`

### Adding Data Types
1. Add to `vtype_t` enum in `value.h`
2. Add union field in `value_t`
3. Update `value_alloc()`, `value_release()`, etc.
4. Implement type-specific functions

### Adding Special Forms
1. Add case in `vm_eval()` in `vm.c`
2. Handle syntax and evaluation logic

### Improving Performance
- Add bytecode compilation
- Implement tail call optimization
- Add JIT compilation (advanced)

### Enhancing Safety
- Add cycle detection to reference counting
- Implement bounds checking for vectors/hashes
- Add type checking for function arguments

## Future Tasks

### High Priority
- [ ] Add more Scheme primitives (let*, begin, set!, etc.)
- [ ] Implement proper tail recursion
- [ ] Add string manipulation functions
- [ ] Improve error messages with line numbers
- [ ] Add REPL mode

### Medium Priority
- [ ] File I/O functions (read-file, write-file)
- [ ] More JSON features (pretty printing, streaming)
- [ ] Macro system (define-syntax, syntax-rules)
- [ ] Module/import system
- [ ] Bytecode compilation for faster execution

### Low Priority
- [ ] Multithreading support
- [ ] Foreign function interface (FFI)
- [ ] Debugger/profiler
- [ ] Standard library extensions
- [ ] Optimization passes (constant folding, etc.)
- [ ] Serialization (save/load VM state)

### Experimental
- [ ] JIT compilation
- [ ] Concurrent garbage collector
- [ ] WebAssembly compilation target
- [ ] Database integration
- [ ] Networking primitives

## Building and Installation

```bash
git clone <repo>
cd pscm
make          # Builds libpscm.a
make clean    # Clean build files
```

Link with `-lpscm` and include `pscm.h`.

## License

[Choose appropriate license - e.g., MIT, BSD, etc.]

## Contributing

1. Keep code clean and well-commented
2. Add tests for new features
3. Follow existing naming conventions
4. Update documentation

For questions or issues, please file at the project repository.