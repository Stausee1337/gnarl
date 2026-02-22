
#include <string>

#include "assertions.h"

namespace gnarl {

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
            ABORT("call to feed in Ended state");
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

}
