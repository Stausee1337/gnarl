
#include <string>
#include "error.h"
#include "token.h"
#include "value.h"
#include "nodes.h"
#include "assertions.h"

namespace gnarl {

Value parse_integer(const std::string_view& view, Error* error) {
    *error = Error("Actual integer parsing isn't implemented yet");
    return Value();
}

Value parse_string(const std::string_view& view, Error* error) {
    *error = Error("Actual string parsing isn't implemented yet");
    return Value();
}

Value LiteralNode::evaluate(Scope* scope, Error* error) const {
    switch (m_tok.kind()) {
        case TokenKind::True:
            return true;
        case TokenKind::False:
            return false;
        case TokenKind::Integer:
            return parse_integer(m_tok.value(), error);
        case TokenKind::String:
            return parse_string(m_tok.value(), error);

        default:
            ABORT("invalid token kind in LiteralNode");
    }
}

class StringParser final {
public:
    enum class QuoteKind {
        Single,
        Double,
    };

    StringParser(QuoteKind quote_kind)
        : quote_kind(quote_kind)
    {}

    void feed(char c);
    bool is_ended() const { return state == State::Ended; }

private:
    void normal(char c);
    void escape(char c);
    void format(char c);

    enum class State {
        Normal,
        Escape,
        Format,
        Ended,
    };

    QuoteKind quote_kind;
    State state = State::Normal;
    std::string buffer;
};

void StringParser::feed(char c) {
    switch (state) {
        case State::Normal:
            normal(c);
        case State::Escape:
            escape(c);
        case State::Format:
            format(c);
        case State::Ended:
            ABORT("call to feed in Ended state");
    }
}

void StringParser::normal(char c) {
    switch (c) {
        case '\\':
            state = State::Escape;
            break;
        case '$':
            state = State::Format;
            break;
        case '\'':
            if (quote_kind == QuoteKind::Single)
                state = State::Ended;
            break;
        case '"':
            if (quote_kind == QuoteKind::Double)
                state = State::Ended;
            break;
        default:
            buffer.push_back(c);
    }
}

void StringParser::escape(char c) {
    state = State::Normal;
    bool is_known = false;
#define known is_known = true; break;

    switch (c) {
        case '\\':
        case '$':
            known;
        case '\'':
            if (quote_kind == QuoteKind::Single)
                known;
            break;
        case '"':
            if (quote_kind == QuoteKind::Double)
                known;
            break;
    }

    if (!is_known)
        buffer.push_back('\\');
    buffer.push_back(c);
}

}

