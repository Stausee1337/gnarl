#ifndef GNARL_TOKEN_H_
#define GNARL_TOKEN_H_

#include <string_view>

#include "position.h"

namespace gnarl {

enum class TokenKind {
    Error,
    EOS,

    String,
    Integer,

    Identifier,
    If,
    Else,
    True,
    False,

    Equal,
    Plus,
    Minus,
    PlusEquals,
    MinusEquals,
    EqualEqual,
    NotEqual,
    LessEqual,
    GreaterEqual,
    LessThan,
    GreaterThan,
    BooleanAnd,
    BooleanOr,
    Bang,
    Dot,
    Comma,

    LParen,
    RParen,
    LBracket,
    RBracket,
    LCurly,
    RCurly,

    UnknownOp,

    LineComment,
    SuffixComment,
    BlockComment,
};

struct Token {
    Token() = default;

    Token(TokenKind kind, std::string_view value, Position position)
        : m_kind(kind),
        m_value(value),
        m_position(position)
    { }

    TokenKind kind() const {
        return m_kind;
    }

    const std::string_view& value() const {
        return m_value;
    }

    const Position& position() const {
        return m_position;
    }

    Span span() const {
        return Span(position(), Position(nullptr, position().lineno(), position().column() + m_value.length()));
    }

private:
    friend class Lexer;

    TokenKind m_kind = TokenKind::Error;
    std::string_view m_value;
    Position m_position;
};

}

#endif // GNARL_TOKEN_H_

