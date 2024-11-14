#include "pengc.h"

#include <assert.h>
#include <string.h>

static bool lexer_is_at_end(struct lexer* lexer) {
    return lexer->offset >= lexer->source_length;
}

static char lexer_current(struct lexer* lexer) {
    if (lexer_is_at_end(lexer)) {
        return 0;
    }

    return lexer->source_text[lexer->offset];
}

static struct location lexer_location(struct lexer* lexer) {
    return (struct location){
        .name = lexer->source_name,
        .text = lexer->source_text,
        .offset = lexer->offset,
        .line = lexer->line,
        .column = lexer->column
    };
}

static void lexer_advance(struct lexer* lexer) {
    if (lexer_is_at_end(lexer)) {
        return;
    }

    if (lexer->source_text[lexer->offset] == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }

    lexer->offset++;
}

void lexer_init(struct lexer* lexer, const char* name, const char* text) {
    memset(lexer, 0, sizeof *lexer);
    lexer->source_name = name;
    lexer->source_text = text;
    lexer->source_length = (isize)strlen(text);
    lexer->line = 1;
    lexer->column = 1;
}

struct token lexer_read_token(struct lexer* lexer) {
    isize begin = lexer->offset;

    if (lexer_is_at_end(lexer)) {
        return (struct token){
            .kind = TK_EOF,
            .location = lexer_location(lexer),
        };
    }

    struct token token = (struct token){
        .kind = TK_INVALID,
        .location = lexer_location(lexer),
    };

    lexer_advance(lexer);

    assert(lexer->offset > begin);
    return token;
}
