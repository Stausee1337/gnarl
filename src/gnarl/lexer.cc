#include <cassert>
#include <cstdlib>
#include <string>
#include <string_view>
#include <unordered_map>

#include "gnarl/lexer.h"

namespace gnarl {

void Lexer::lex(std::vector<Token>& buffer) {
    bump();

    while (true) {
        Token tok = this->lex_one_token();
        buffer.push_back(tok);

        if (tok.m_kind == TokenKind::EOS)
            break;
    }
}

Token Lexer::lex_one_token() {
    eat_whitespace();
    if (is_eos())
        return make_token(TokenKind::EOS);

    char c = current();
    tok_start = position();

    if (c == '#')
        return lex_comment();

    if (c >= '0' && c <= '9')
        return lex_number_literal();

    if (c == '"' || c == '\'')
        return lex_string_literal();

    if (isalpha(c) || c == '_')
        return lex_identifier_or_keyword();

    if (ispunct(c))
        return lex_punct();

    return make_token(TokenKind::Error);
}

Token Lexer::lex_comment() {
    bump();
    char c = current();
    do {
        bump();
        c = current();
    } while (c != '\n' || c == '\r');
    return make_token(TokenKind::Comment);
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


class StringParser final {
public:
    enum class QuoteKind {
        Single,
        Double,
    };

    StringParser(QuoteKind quote_kind)
        : quote_kind_(quote_kind)
    {}

    void feed(char c);
    bool is_ended() const { return state_ == State::Ended; }

private:
    void normal(char c);
    void escape(char c);

    enum class State {
        Normal,
        Escape,
        Ended,
    };

    QuoteKind quote_kind_;
    State state_ = State::Normal;
    std::string buffer;
};

void StringParser::feed(char c) {
    switch (state_) {
        case State::Normal:
            normal(c);
        case State::Escape:
            escape(c);
        case State::Ended:
            assert(false && "call to feed in Ended state");
    }
}

void StringParser::normal(char c) {
    switch (c) {
        case '\\':
            state_ = State::Escape;
            break;
        case '\n':
        case '\r':
            state_ = State::Ended;
            // *err = Err("String literal is unclosed");
            break;
        case '\'':
            if (quote_kind_ == QuoteKind::Single)
                state_ = State::Ended;
            break;
        case '"':
            if (quote_kind_ == QuoteKind::Double)
                state_ = State::Ended;
            break;
        default:
            buffer.push_back(c);
    }
}

void StringParser::escape(char c) {
    state_ = State::Normal;
    switch (c) {
        case '\\': buffer.push_back('\\');
        case '\'':
            if (quote_kind_ == QuoteKind::Single)
                buffer.push_back('\'');
        case '"':
            if (quote_kind_ == QuoteKind::Double)
                buffer.push_back('"');
    }
    buffer.push_back(c);
}

Token Lexer::lex_string_literal() {
    char c = current();
    char end = c;

    do {
        bump();
        c = current();

        if (c == '\\')
            continue;

    } while (c != end && c != '\n' && c != '\r');

    if (c != end) {
        // *err = Err("unclosed stirng literal");
        return make_token(TokenKind::Error);
    }

    return make_token(TokenKind::String);
}

Token Lexer::lex_number_literal() {
    char c = current();
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
    return Token(kind, data, make_position());
}

Position Lexer::make_position() const {
    return Position(&source_file, lineno, position() - bol);
}

void Lexer::eat_whitespace() {
    char c = current();
    while(isspace(c)) {
        if (c == '\r' || c == '\n') {
            if (c == '\r' && next() == '\n')
                bump();
            lineno++;
            bol = position();
        }
        bump();
    }
}

bool Lexer::is_eos() const {
    return m_position > source.length();
}

char Lexer::next() const {
    if (m_position > source.length()) {
        return 0;
    }
    return source[m_position + 1];
}

void Lexer::bump() {
    if (m_position > source.length()) {
        m_current = 0;
        return;
    }
    m_current = source[++m_position];
}

std::vector<Token> Lexer::lex_to_buffer(const SourceFile& source_file) {
    std::vector<Token> buffer;
    if (source_file.is_empty()) return buffer;

    Lexer lexer(source_file);
    lexer.lex(buffer);
    return buffer;
}

}


