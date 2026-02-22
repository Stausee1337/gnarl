
#ifndef GNARL_LEXER_H_
#define GNARL_LEXER_H_

#include <math.h>
#include <stdint.h>
#include <assert.h>
#include <limits>
#include <string_view>
#include <vector>

#include "gnarl/token.h"
#include "gnarl/source.h"

namespace gnarl {

class Lexer final {
public:
    static std::vector<Token> lex_to_buffer(const SourceFile& source_file);

private:
    Lexer(const SourceFile& source_file)
        : source(source_file.source()),
        source_file(source_file)
    {}

    void lex(std::vector<Token>& buffer);

    Token lex_one_token();
    void eat_whitespace();
    Token lex_comment();
    Token lex_string_literal();
    Token lex_number_literal();
    Token lex_punct();
    Token lex_identifier_or_keyword();

    Token make_token(TokenKind kind) const;
    Position make_position() const;

    void bump();
    bool is_eos() const;
    char next() const;

    size_t position() const { return m_position - 1; }
    char current() const { return m_current; }

    char m_current = 0;

    uint32_t lineno = 1;
    size_t bol = 0;
    size_t tok_start;
    size_t m_position = 0;

    std::string_view source;
    const SourceFile& source_file;
};

}

#endif // GNARL_LEXER_H_

