
#ifndef GNARL_FORMAT_H_
#define GNARL_FORMAT_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

#include "assertions.h"
#include "integer.h"

namespace gnarl {

class Formatter final {
public:
    Formatter(FILE* file) : m_file(file)
    {}

    void write(const char* data, size_t size);
    void flush();

private:
    FILE* m_file;
};

template<typename T>
struct FormatTrait {
    using __type_non_formattable = void;
};

template<typename T, typename = void>
constexpr bool is_formatable = true;

template<typename T>
constexpr bool is_formatable<T, typename FormatTrait<T>::__type_non_formattable> = false;

template<typename T>
concept Formatable = is_formatable<T>;

template <Formatable T>
void generic_format_fn(const void* self_erased, Formatter& fmt) {
    const T* self = static_cast<const T*>(self_erased);
    FormatTrait<T>::fmt(*self, fmt);
}

class TypeErasedParameter final {
public:
    enum Type {
        INTEGER,
        UNSIGNED_INTEGER,
        CHAR,
        STRING_VIEW,
        FORMATABLE,
    };

    struct ErasedFormatable {
        const void* m_value;
        void(*m_format_fn)(const void* self, Formatter& fmt);
    };

    template<Integer Int>
        requires (!is_unsigned<Int> && !is_char<Int>)
    TypeErasedParameter(Int value)
        : m_type(Type::INTEGER),
        m_integer(value)
    { }

    template<Integer Int>
        requires (is_unsigned<Int>)
    TypeErasedParameter(Int value)
        : m_type(Type::UNSIGNED_INTEGER),
        m_uinteger(value)
    { }

    template<Formatable T>
    TypeErasedParameter(const T& formatable)
        : m_type(Type::FORMATABLE),
        m_formatable { .m_value = &formatable, .m_format_fn = generic_format_fn<T> }
    { }

    TypeErasedParameter(char c)
        : m_type(Type::CHAR),
        m_char(c)
    {}

    TypeErasedParameter(const std::string_view& string_view)
        : m_type(Type::STRING_VIEW),
        m_string_view(string_view)
    {}

    Type type() const { return m_type; }
    void format(Formatter& fmt) const;

    int64_t integer() const {
        return m_integer;
    }

    uint64_t uinteger() const {
        return m_uinteger;
    }

private:
    Type m_type;
    union {
        char m_char;
        int64_t m_integer;
        uint64_t m_uinteger;
        std::string_view m_string_view;
        ErasedFormatable m_formatable;
    };
};

template<>
struct FormatTrait<std::string> {
    static void fmt(const std::string& self, Formatter& fmt);
};

class TypeErasedParameterArray {
public:
    TypeErasedParameterArray(uint32_t size) : m_size(size)
    {}

    uint32_t size() const { return m_size; }
    TypeErasedParameter operator[](size_t idx) const {
        DCHECK(idx < size());
        return m_parameters[idx];
    }

private:
    uint32_t m_size;
    TypeErasedParameter m_parameters[0];
};

template<typename ...Parameters>
class ParameterArray final : public TypeErasedParameterArray {
public:
    ParameterArray(Parameters const&... parameters)
        : TypeErasedParameterArray(sizeof...(parameters)),
        m_storage { TypeErasedParameter(parameters)... }
    { }

private:
    TypeErasedParameter m_storage[sizeof...(Parameters)];
};

struct FixedString final {
    template<size_t N>
    FixedString(const char (&data)[N])
        : m_length(N - 1),
        m_data(data)
    { }

    size_t size() const { return m_length; }
    const char* data() const { return m_data; }

    operator std::string_view() const {
        return std::string_view(data(), size());
    }

private:
    size_t m_length;
    const char* m_data;
};

void format_impl(Formatter& fmt, FixedString message, const TypeErasedParameterArray& parameters);

template<typename ...Args>
inline void print(FixedString message, Args... args) {
    Formatter formatter(stdout);
    format(formatter, message, args...);
}

template<typename ...Args>
inline void format(Formatter& fmt, FixedString message, Args... args) {
    ParameterArray<Args...> parameters(args...);
    format_impl(fmt, message, parameters);
}

}

#endif // GNARL_FORMAT_H_

