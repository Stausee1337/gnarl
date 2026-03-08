
#ifndef GNARL_INTEGER_H_
#define GNARL_INTEGER_H_

#include <stdlib.h>
#include <stdint.h>

namespace gnarl {

template<typename T>
inline constexpr bool is_integer = false;

template<typename T>
inline constexpr bool is_unsigned = false;

#define ENUMERATE_SIGNED_INTEGERS(X)    \
    X(char) X(short) X(int)             \
    X(long) X(long long)

#define DECL_SIGNED_INTEGERS(int)   \
    template<>                      \
    inline constexpr bool is_integer<int> = true;

#define DECL_UNSIGNED_INTEGERS(int)                         \
    template<>                                              \
    inline constexpr bool is_integer<unsigned int> = true;  \
    template<>                                              \
    inline constexpr bool is_unsigned<unsigned int> = true;

ENUMERATE_SIGNED_INTEGERS(DECL_SIGNED_INTEGERS)
ENUMERATE_SIGNED_INTEGERS(DECL_UNSIGNED_INTEGERS)

#undef DECL_UNSIGNED_INTEGERS
#undef DECL_SIGNED_INTEGERS
#undef ENUMERATE_SIGNED_INTEGERS

template<typename T>
concept Integer = is_integer<T>;

template<typename T>
inline constexpr bool is_char = false;

template<>
inline constexpr bool is_char<char> = true;

bool u16_from_ascii(const char* begin, size_t length, uint16_t* result);
bool i64_from_ascii(const char* begin, size_t length, int64_t* result);

}

#endif // GNARL_INTEGER_H_

