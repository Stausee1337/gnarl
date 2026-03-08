
#include <string.h>
#include <optional>
#include <string_view>
#include <variant>

#include "format.h"

namespace gnarl {

void Formatter::write(const char* data, size_t size) {
    fwrite(data, size, 1, m_file);
}

void Formatter::flush() {
    fflush(m_file);
}

void TypeErasedParameter::format(Formatter& fmt) const {
    switch (m_type) {
        case Type::INTEGER:
        {
            std::string str = std::to_string(m_integer);
            fmt.write(str.data(), str.size());
        }
        break;
        case Type::UNSIGNED_INTEGER:
        {
            std::string str = std::to_string(m_uinteger);
            fmt.write(str.data(), str.size());
        }
        break;
        case Type::CHAR:
            fmt.write(&m_char, 1);
            break;
        case Type::STRING_VIEW:
            fmt.write(m_string_view.data(), m_string_view.size());
            break;
        case Type::FORMATABLE:
            m_formatable.m_format_fn(m_formatable.m_value, fmt);
            break;
    }
}

void FormatTrait<std::string>::fmt(const std::string& self, Formatter& fmt) {
    fmt.write(self.data(), self.size());
}

class FormatStringParser final {
public:
    struct Argument {
        std::optional<char> fill;
        bool pad_left;
        ssize_t fill_width;
    };

    struct Segment {
        Segment() = default;

        Segment(std::string_view data)
            : m_variant(data)
        {}

        Segment(Argument argument)
            : m_variant(argument)
        {}

        bool is_empty() const {
            return std::holds_alternative<std::monostate>(m_variant);
        }

        const std::string_view* as_string() {
            if (std::holds_alternative<std::string_view>(m_variant))
                return &std::get<std::string_view>(m_variant);
            return nullptr;
        }

        const Argument* as_argument() {
            if (std::holds_alternative<Argument>(m_variant))
                return &std::get<Argument>(m_variant);
            return nullptr;
        }

    private:
        std::variant<std::monostate, std::string_view, Argument> m_variant;
    };

    using Iterator = std::string_view::const_iterator;

    
    FormatStringParser(std::string_view data)
        : m_current(data.begin()),
        m_data(data)
    {}

    bool advance(Segment& segment) {
        if (m_current >= m_data.end()) return false;

        char c = *m_current;
        switch (c) {
            case '{':
                m_current++;
                if (m_current < m_data.end() && *m_current == '{') {
                    m_current++;
                    segment = string();
                } else {
                    segment = argument();
                    if (*(m_current++) != '}') {
                        ABORT("missing closing brace `}` in format string literal");
                    }
                }
                break;
            default:
                segment = string();
        }
        return true;
    }

private:
    Segment string() {
        Iterator start = m_current++;
        for (; m_current != m_data.end(); ++m_current) {
            char c = *m_current;
            if (c == '{') break;
        }
        return std::string_view(&*start, m_current - start);
    }

    Segment argument() {
        Argument arg;
        consume_whitespace();
        if (*m_current != ':')
            return arg;
        char c = *(++m_current);

        if ((m_current + 1) < m_data.end() && (*(m_current + 1) == '>' || *(m_current + 1) == '<')) {
            arg.fill = c;
            arg.pad_left = *(++m_current) == '>';
        } else {
            DCHECK(c == '>' || c == '<');
            arg.fill = ' ';
            arg.pad_left = c == '>';
        }

        if (*(++m_current) == '$') {
            m_current++;
            arg.fill_width = -1;
            return arg;
        }

        arg.fill_width = integer();

        return arg;
    }

    void consume_whitespace() {
        char c = *m_current;
        while(isspace(c)) {
            c = *(++m_current);
        }
    }

    uint16_t integer() {
        char c = *m_current;
        DCHECK(isdigit(c));

        Iterator start = m_current++;
        while(isdigit(c) && m_current < m_data.end()) {
            c = *(++m_current);
        }
        uint16_t result;
        DCHECK(u16_from_ascii(&*start, m_current - start, &result));

        return result;
    }

    Iterator m_current;
    std::string_view m_data;
};

void format_impl(Formatter& fmt, FixedString string, const TypeErasedParameterArray& parameters) {
    FormatStringParser parser(string);

    size_t current_param = 0;

    FormatStringParser::Segment seg;
    while(parser.advance(seg)) {
        if (auto string = seg.as_string()) {
            fmt.write(string->data(), string->size());
            continue;
        }

        DCHECK(!seg.is_empty());
        const auto& argument = *seg.as_argument();

        TypeErasedParameter format_param = parameters[current_param++];
        if (!argument.fill.has_value()) {
            format_param.format(fmt);
            continue;
        }
        char fill_char = argument.fill.value();

        ssize_t padding_length = argument.fill_width;
        if (argument.fill_width < 0) {
            TypeErasedParameter fill_width_param = parameters[current_param++];
            switch (fill_width_param.type()) {
                case TypeErasedParameter::INTEGER:
                    padding_length = fill_width_param.integer();
                    break;
                case TypeErasedParameter::UNSIGNED_INTEGER:
                    padding_length = fill_width_param.uinteger();
                    break;
                default:
                    ABORT("invalid parameter type");
            }
        }
        char* padding = new char[padding_length];
        memset(padding, fill_char, padding_length);

        if (argument.pad_left)
            fmt.write(padding, padding_length);

        format_param.format(fmt);

        if (!argument.pad_left)
            fmt.write(padding, padding_length);

        // FIXME: use static memory for padding buffer
        // for sizes up to 255
        delete[] padding;
    }
}

}

