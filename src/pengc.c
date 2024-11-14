#include <stdio.h>

#include "pengc.h"

int main(int argc, char** argv) {
    fprintf(stderr, "Hello, hunter!\n");

    struct lexer lexer = {0};
    lexer_init(&lexer, "foo.peng", "34 35 +\nprint");

    struct token token = {0};
    while ((token = lexer_read_token(&lexer)).kind != TK_EOF) {
        fprintf(stderr, "(%"PRIi64", %"PRIi64"): %c\n", token.location.line, token.location.column, token.location.text[token.location.offset]);
    }

    return 0;
}
