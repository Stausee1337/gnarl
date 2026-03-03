
#include <sstream>
#include <string>
#include <variant>
#include "error.h"
#include "token.h"
#include "value.h"
#include "scope.h"
#include "nodes.h"
#include "assertions.h"

namespace gnarl {
    
bool i64_from_ascii(const char* begin, size_t length, int64_t* result);
std::string expand_string_literal(const Token& token, const Scope* scope, Error* error);

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
        {
            std::string strval = expand_string_literal(m_tok, scope, error);
            if (error->has_error())
                return Value();
            return Value(std::move(strval));
        }
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
                result = Derived::addition(result, c - '0');
            }
        } else {
            while (begin != end) {
                char c = *(begin++);
                if (c < '0' || c > '9') return false;

                if (__builtin_mul_overflow(result, 10, &result)) return false;
                if (Derived::checked_addition(result, c - '0', &result)) return false;
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

bool hexdigit(char c, char* result) {
    if (c >= '0' && c <= '9') {
        *result |= (c - '0');
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        *result |= (c - 'A') + 10;
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        *result |= (c - 'a') + 10;
        return true;
    }

    return false;
}

class StringParser final {
public:
    struct Segment final {
        enum Kind {
            DATA,
            INTERPOLATED_IDENT,
            INTERPOLATED_EXPR,
            END,
        };

        Kind kind() const { return m_kind; }
        const std::string_view& string() const { return m_string; }

        Span span() const {
            return Span(m_position, Position(nullptr, m_position.lineno(), m_position.column() + m_string.length()));
        }
        
    private:
        Kind m_kind;
        Position m_position;
        std::string_view m_string;
        
        template<Kind KIND>
        struct Initializer;

#define DEFINE_COMMON_INITIALIZER(Kind)                                                             \
        template<>                                                                                  \
        struct Initializer<Kind> {                                                                  \
            static void initialize(Segment& segment, Position position, std::string_view string) {  \
                segment.m_position = position;                                                      \
                segment.m_string = string;                                                          \
            }                                                                                       \
        }
DEFINE_COMMON_INITIALIZER(DATA);
DEFINE_COMMON_INITIALIZER(INTERPOLATED_IDENT);
DEFINE_COMMON_INITIALIZER(INTERPOLATED_EXPR);
#undef DEFINE_COMMON_INITIALIZER

        template<>
        struct Initializer<END> {
            static void initialize(Segment& segment) {}
        };

        Segment(Kind kind) : m_kind(kind) {}

    public:

        template<Kind KIND, typename... Args>
        static Segment from_kind(Args... args) {
            Segment result(KIND);
            Initializer<KIND>::initialize(result, args...);
            return result;
        }
    };

    struct Iterator final {
        Iterator(Segment segment, StringParser* parser)
            : m_segment(segment),
            m_parser(parser)
        {}

        const Segment& operator*() const { return m_segment; }

        Iterator& operator++() {
            m_segment = m_parser->advance();
            return *this;
        }

        bool operator==(const Iterator& other) const {
            return m_parser == other.m_parser && m_segment.kind() == other.m_segment.kind();
        }

    private:
        Segment m_segment;
        StringParser* m_parser;
    };

    StringParser(const Token& token, Error* error)
        : position(token.position()),
        error(error)
    {
        std::string_view value = token.value();
        DCHECK(value.length() >= 2);

        literal_begin = value.data();
        literal_end = literal_begin + value.length() - 1;

        current_char = literal_begin + 1;
    }


    Iterator begin() {
        return Iterator(advance(), this);
    }

    Iterator end() {
        return Iterator(Segment::from_kind<Segment::END>(), this);
    }

private:
    enum State {
        Detect,
        Normal,
        Escape,
        SpecialDectect,
        ArbitraryChar,
        FormatFlat,
        FormatBlock,
        Done,
    };


    Segment advance() {
        using StateFn = State(StringParser::*)();
        constexpr StateFn functions[] = {
            &StringParser::detect,
            &StringParser::normal,
            &StringParser::escape,
            &StringParser::special_detect,
            &StringParser::arbitrary_char,
            &StringParser::format_flat,
            &StringParser::format_block,
        };

        if (current_char == literal_end)
            return  Segment::from_kind<Segment::END>();

        error_begin = current_char;

        State s = State::Detect;
        while (s < State::Done) {
            s = (this->*functions[s])();
        }

        if (error->has_error())
            return Segment::from_kind<Segment::END>();
        return m_segment;
    }

    State detect() {
        segment_begin = current_char;
        switch (*current_char) {
            case '\\':
                return State::Escape;
            case '$':
                return State::SpecialDectect;
            default:
                return State::Normal;
        }
    }

    State normal() {
        char c = *current_char;
        while (c != '\\' && c != '$' && current_char < literal_end) {
            c = *(++current_char);
        }

        bind_segment<Segment::DATA>();
        return State::Done;
    }

    State escape() {
        char c = *(current_char + 1);
        if (c == '\\' || c == '$' || c == '"')
            segment_begin = ++current_char;
        else
            segment_begin = current_char;
        current_char++;
        return State::Normal;
    }

    State special_detect() {
        segment_begin = ++current_char;
        if (current_char == literal_end) {
            make_error("$ at the end of string",
                       "I was expecting an identifier, 0xFF, or {...} after the $");
            return State::Done;
        }
        char c = *current_char;
        if (c == '{') {
            segment_begin = ++current_char;
            return State::FormatBlock;
        } else if (c == '0') {
            current_char++;
            return State::ArbitraryChar;
        }
        return State::FormatFlat;
    }

    State arbitrary_char() {
#define error_out()                                                                 \
        do {                                                                        \
            make_error("Invalid hex character. Hex values must look like 0xFF");    \
            return State::Done; \
        } while (0)

#define is_end() (current_char == literal_end)

        char c = *current_char;
        if (is_end() || c != 'x')
            error_out();

        c = *(++current_char);
        if (is_end()) error_out();

        m_storage = 0;
        if (!hexdigit(c, &m_storage)) error_out();

        c = *(++current_char);
        if (is_end()) error_out();

        m_storage <<= 4;
        if (!hexdigit(c, &m_storage)) error_out();

        current_char++;
        bind_segment(std::string_view(&m_storage, 1));

        return State::Done;
#undef is_end
#undef error_out
    }

    State format_flat() {
        char c = *current_char;
        if (!(isalpha(c) || c == '_')) {
            make_error("$ not followed by an identifier char",
                       "If you want a literal $ use \"\\$\"");
            return State::Done;
        }
        while ((isalnum(c) || c == '_') && current_char < literal_end) {
            c = *(++current_char);
        }
        bind_segment<Segment::INTERPOLATED_IDENT>();
        return State::Done;
    }

    State format_block() {
        char c = *current_char;

        bool simple_identifier = true;
        while (current_char < literal_end && c != '}') {
            simple_identifier &= (isalnum(c) || c == '_');
            c = *(++current_char);
        }
        if (current_char == literal_end) {
            make_error("Unterminated ${...");
            return State::Done;
        }
        DCHECK(c == '}');

        if (simple_identifier)
            bind_segment<Segment::INTERPOLATED_IDENT>();
        else
            bind_segment<Segment::INTERPOLATED_EXPR>();
        current_char++;
        return State::Done;
    }

    void make_error(std::string message, std::string help = std::string()) {
        size_t offset = error_begin - literal_begin;
        Position begin(position.file(), position.lineno(), position.column() + offset);


        size_t length = current_char - error_begin;
        Span span(begin, Position(nullptr, begin.lineno(), begin.column() + length));
        *error = Error(span, message, help);
    }

    template<Segment::Kind KIND>
    void bind_segment() {
        size_t length = current_char - segment_begin;
        std::string_view data(segment_begin, length);

        size_t offset = segment_begin - literal_begin;
        Position current_position(position.file(), position.lineno(), position.column() + offset);

        m_segment = Segment::from_kind<KIND>(current_position, data);
    }

    void bind_segment(std::string_view data) {
        size_t offset = segment_begin - literal_begin;
        Position current_position(position.file(), position.lineno(), position.column() + offset);

        m_segment = Segment::from_kind<Segment::DATA>(current_position, data);
    }

    Segment m_segment = Segment::from_kind<Segment::END>();
    Position position;
    Error* error;

    char m_storage;

    const char* literal_begin;
    const char* literal_end;

    const char* current_char;
    const char* segment_begin;
    const char* error_begin;
};

std::string expand_string_literal(const Token& token, const Scope* scope, Error* error) {
    StringParser p(token, error);

    std::stringstream result;

    for (auto seg : p) {
        switch (seg.kind()) {
            case StringParser::Segment::DATA:
                result << seg.string();
                break;
            case StringParser::Segment::INTERPOLATED_IDENT:
            {
                const std::string_view& name = seg.string();
                const Value* value = scope->get_value(name);
                if (!value) {
                    *error = Error(seg.span(), "Undefined identifier in string expansion", "\"" + std::string(name) + "\" is currently not in scope");
                    return std::string();
                }
                result << value->stringify();
            }
            break;
            case StringParser::Segment::INTERPOLATED_EXPR:
            {
                ABORT("not implemented");
            }
            break;
            default:
                break;
        }
    }

    if (error->has_error())
        return std::string();

    return result.str();
}

}

