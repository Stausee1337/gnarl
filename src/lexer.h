
#ifndef GNARL_LEXER_H_
#define GNARL_LEXER_H_

#include <stdint.h>
#include <limits>
#include <string_view>
#include <vector>

#include "token.h"
#include "input_file.h"
#include "error.h"

namespace gnarl {

class Lexer final {
public:
    static std::vector<Token> lex_to_buffer(
            const InputFile& input_file, Error* error);

private:
    Lexer(const InputFile& input_file, Error* error)
        : error(error),
        source(input_file.source()), 
        input_file(input_file)
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
    size_t bol = -1;
    size_t tok_start;
    size_t m_position = 0;

    Error* error;
    std::string_view source;
    const InputFile& input_file;
};

}

#endif // GNARL_LEXER_H_

