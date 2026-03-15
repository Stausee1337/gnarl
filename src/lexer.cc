#include <stdlib.h>
#include <string>
#include <string_view>

#include "lexer.h"
#include "assertions.h"
#include "token.h"

namespace gnarl {

void Lexer::lex(std::vector<Token>& buffer) {
    m_current = source[m_position];

    while (true) {
        Token tok = lex_one_token();
        if (tok.kind() == TokenKind::EOS || tok.kind() == TokenKind::Error)
            break;

        buffer.push_back(tok);
        previous = tok;
    }
}

Token Lexer::lex_one_token() {
    eat_whitespace();
    tok_start = position();
    // printf("Is eof: %d\n", is_eof());

    if (is_eof())
        return make_token(TokenKind::EOS);

    char c = current();

    if (c == '#')
        return lex_comment();

    if (isdigit(c) || (c == '-' && isdigit(next())))
        return lex_number_literal();

    if (c == '"')
        return lex_string_literal();

    if (c == '\'') {
        *error = Error(current_position(),
                       "Invalid token",
                       "Strings are delimited by \" characters, not apostrophes");
        return make_token(TokenKind::Error);
    }

    if (isalpha(c) || c == '_')
        return lex_identifier_or_keyword();

    // FIXME: `ispunct` probably isn't selective enough here (referring to original gn)
    if (ispunct(c))
        return lex_punct();

    *error = Error(current_position(), "Invalid token", "I have no idea what this is");
    return make_token(TokenKind::Error);
}

Token Lexer::lex_comment() {
    bump();
    char c = current();
    while (c != '\n' && c != '\r' && !is_eof()) {
        bump();
        c = current();
    }

    TokenKind kind = TokenKind::SuffixComment;

    // Adapted from gn/tokenizer.cc - Detection of Line and BlockComment
    if (at_start_of_line(tok_start) && 
        (!previous.has_value()
            || previous->kind() != TokenKind::SuffixComment
            || previous->position().lineno() + 1 != lineno
            || previous->position().column() != column())) {
        kind = TokenKind::LineComment;
        if (!is_eof()) { // \n or \r
            if (c == '\r' && next() == '\n')
                bump();
            lineno++;
            bol = position() + 1;
            bump();
        }

        while (isspace(c)) {
            if (c == '\r' || c == '\n') {
                kind = TokenKind::BlockComment;
                break;
            }
            bump();
            c = current();
        }
    }

    return make_token(kind);
}

Token Lexer::lex_identifier_or_keyword() {
    char c; 
    do {
        bump();
        c = current();
    } while (isalnum(c) || c == '_');

    Token token = make_token(TokenKind::Identifier);
    const std::string_view& value = token.value();

    if (value == "if")
        token.m_kind = TokenKind::If;
    else if (value == "else")
        token.m_kind = TokenKind::Else;
    else if (value == "true")
        token.m_kind = TokenKind::True;
    else if (value == "false")
        token.m_kind = TokenKind::False;
    return token;
}

Token Lexer::lex_punct() {
    char c0 = current();
    bump();
    char c1;
    switch(c0) {
        case '.':
            return make_token(TokenKind::Dot);
        case ',':
            return make_token(TokenKind::Comma);
        case '[':
            return make_token(TokenKind::LBracket);
        case ']':
            return make_token(TokenKind::RBracket);
        case '{':
            return make_token(TokenKind::LCurly);
        case '}':
            return make_token(TokenKind::RCurly);
        case '(':
            return make_token(TokenKind::LParen);
        case ')':
            return make_token(TokenKind::RParen);
        case '=':
            c1 = current();
            if (c1 == '=') {
                bump();
                return make_token(TokenKind::EqualEqual);
            }
            return make_token(TokenKind::Equal);
        case '!':
            c1 = current();
            if (c1 == '=') {
                bump();
                return make_token(TokenKind::NotEqual);
            }
            return make_token(TokenKind::Bang);
        case '&':
            c1 = current();
            if (c1 == '&') {
                bump();
                return make_token(TokenKind::BooleanAnd);
            }
            break;
        case '|':
            c1 = current();
            if (c1 == '|') {
                bump();
                return make_token(TokenKind::BooleanOr);
            }
            break;
        case '+': 
            c1 = current();
            if (c1 == '=') {
                bump();
                return make_token(TokenKind::PlusEquals);
            }
            return make_token(TokenKind::Plus);
        case '-': 
            c1 = current();
            if (c1 == '=') {
                bump();
                return make_token(TokenKind::MinusEquals);
            }
            return make_token(TokenKind::Minus);
        case '<':
            c1 = current();
            if (c1 == '=') {
                bump();
                return make_token(TokenKind::LessEqual);
            }
            return make_token(TokenKind::LessThan);
        case '>':
            c1 = current();
            if (c1 == '=') {
                bump();
                return make_token(TokenKind::GreaterEqual);
            }
            return make_token(TokenKind::GreaterThan);
        break;
    }

    return make_token(TokenKind::UnknownOp);
}

Token Lexer::lex_string_literal() {
    char c = current();

    do {
        bump();
        c = current();

        if (c == '\\') {
            bump();
            continue;
        }

    } while (c != '"' && c != '\n' && c != '\r' && !is_eof());

    if (c == '"') {
        bump();
        return make_token(TokenKind::String);
    }

    if (is_eof())
        *error = Error(token_span(),
                      "Unterminated string literal",
                      "Don't leave me hanging like this!");
    else
        *error = Error(token_span(), "Newline in string constant");
    bump();
    return make_token(TokenKind::Error);
}

Token Lexer::lex_number_literal() {
    char c = current();
    if (c == '-')
        bump();
    bump();

    while (true) {
        c = current();
        if (c >= '0' && c <= '9')
            bump();
        else
            break;
    }
    return make_token(TokenKind::Integer);
}

Token Lexer::make_token(TokenKind kind) const {
    std::string_view data(source.data() + tok_start, position() - tok_start);
    Position token_start(&input_file, lineno, (tok_start - bol) + 1);
    return Token(kind, data, token_start);
}

Position Lexer::current_position() const {
    return Position(&input_file, lineno, column());
}

Span Lexer::token_span() const {
    Position token_start(&input_file, lineno, (tok_start - bol) + 1);
    return Span(token_start, current_position());
}

bool Lexer::at_start_of_line(size_t offset) const {
    DCHECK(offset <= source.length());
    if (offset == 0) return true;
    while (offset > 0) {
        char c = source[--offset];
        if (c == '\n')
            return true;
        if (c != ' ')
            return false;
    }
    return false;
}

void Lexer::eat_whitespace() {
    char c = current();
    while(isspace(c)) {
        if (c == '\r' || c == '\n') {
            if (c == '\r' && next() == '\n')
                bump();
            lineno++;
            bol = position() + 1;
        }
        bump();
        c = current();
    }
}

bool Lexer::is_eof() const {
    return m_position >= source.length();
}

char Lexer::next() const {
    if (m_position >= source.length()) {
        return 0;
    }
    return source[m_position + 1];
}

void Lexer::bump() {
    if (m_position >= source.length()) {
        m_current = 0;
        return;
    }
    m_current = source[++m_position];
}

std::vector<Token> Lexer::lex_to_buffer(const InputFile& input_file, Error* error) {
    std::vector<Token> buffer;
    if (input_file.is_empty()) return buffer;

    Lexer lexer(input_file, error);
    lexer.lex(buffer);
    return buffer;
}


std::vector<Token> Lexer::sublex_to_buffer(const Position& start, std::string_view data, Error* error) {
    std::vector<Token> buffer;
    if (data.empty()) return buffer;

    DCHECK(start.file());
    Lexer lexer(*start.file(), error);
    lexer.source = data;
    lexer.lineno = start.lineno();
    lexer.bol    = -start.column(); // FIXME: very hacky hack 
                                    // (a better way might be to find the offset 
                                    // of data within the full source file string)

    lexer.lex(buffer);
    DCHECK(lexer.lineno == start.lineno());
    return buffer;
}

}


