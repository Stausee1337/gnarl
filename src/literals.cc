
#include <string>
#include "error.h"
#include "token.h"
#include "value.h"
#include "nodes.h"
#include "assertions.h"

namespace gnarl {
    
bool i64_from_ascii(const char* begin, size_t length, int64_t* result);

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
        {
            const std::string_view& value = m_tok.value();
            if ((value.starts_with("0") && value.size() > 1) || value.starts_with("-0")) {
                if (value == "-0")
                    *error = Error(get_span(), "Negative zero doesn't make sense");
                else
                    *error = Error(get_span(), "Leading zeros not allowed");
                return Value();
            }
            int64_t result;
            if (!i64_from_ascii(value.data(), value.size(), &result)) {
                *error = Error(get_span(), "This doesn't look like an integer");
                return Value();
            }
            return result;
        }
        case TokenKind::String:
            return parse_string(m_tok.value(), error);
        default:
            ABORT("invalid token kind in LiteralNode");
    }
}

// Integer parsing algorithm adapted for C++ from:
// https://github.com/rust-lang/rust/blob/b935f37/library/core/src/num/mod.rs#L1708

#define len ((size_t)(end - begin))

template<typename Derived>
struct IntegerParser {
    static bool parse(const char* begin, size_t length, int64_t* out_result) {
        const char* end = begin + length;

        int64_t result = 0;

        bool cannot_overflow = len <= sizeof(int64_t) * 2 - 1;
        if (cannot_overflow) {
            while (begin != end) {
                char c = *(begin++);
                if (c < '0' || c > '9') return false;

                result *= 10;
                result = Derived::addition(result, c);
            }
        } else {
            while (begin != end) {
                char c = *(begin++);
                if (c < '0' || c > '9') return false;

                if (__builtin_mul_overflow(result, 10, &result)) return false;
                if (Derived::checked_addition(result, c, &result)) return false;
            }
        }

        *out_result = result;
        return true;
    }
};

struct Positive : public IntegerParser<Positive> {
    static int64_t addition(int64_t a, int64_t b) {
        return a + b;
    }

    static bool checked_addition(int64_t a, int64_t b, int64_t* result) {
        return __builtin_add_overflow(a, b, result);
    }
};

struct Negative : public IntegerParser<Negative> {
    static int64_t addition(int64_t a, int64_t b) {
        return a - b;
    }

    static bool checked_addition(int64_t a, int64_t b, int64_t* result) {
        return __builtin_sub_overflow(a, b, result);
    }
};

bool i64_from_ascii(const char* begin, size_t length, int64_t* result) {
    if (length == 0) return false;
    const char* end = begin + length;


    bool is_positive = true;
    if (*begin == '+' || *begin == '-') {
        is_positive = *begin == '-';
        begin++;
    }

    if (len == 0) return false;

    if (is_positive)
        return Positive::parse(begin, len, result);
    else
        return Negative::parse(begin, len, result);
}
#undef len

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

