#ifndef PENGC_H
#define PENGC_H

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef ptrdiff_t isize;
typedef size_t usize;

// X(Ident, Singular, Plural)
#define TOKENS(X)                         \
    X(EOF, "end of file", "end of files") \
    X(NAME, "name", "names")              \
    X(INTEGER, "integer", "integers")     \
    X(FLOAT, "float", "floats")           \
    X(STRING, "string", "strings")

#define OPCODES(X)

// clang-format off
enum token_kind {
    TK_INVALID,
#define X(Ident, ...) TK_##Ident,
    TOKENS(X)
#undef X
    TK_COUNT,
};
// clang-format on

struct location {
    const char* name;
    const char* text;

    isize offset;
    i64 line, column;
};

struct token {
    enum token_kind kind;
    struct location location;
    union {
        const char* string_value;
        i64 integer_value;
        f64 float_value;
    };
};

struct lexer {
    const char* source_name;
    const char* source_text;
    isize source_length;

    isize offset;
    i64 line, column;
};

// ========== data.c ==========

const char* token_singular(enum token_kind kind);
const char* token_plural(enum token_kind kind);

// ========== lex.c ==========

void lexer_init(struct lexer* lexer, const char* name, const char* text);
struct token lexer_read_token(struct lexer* lexer);

#endif // PENGC_H
