
#include "integer.h"

namespace gnarl {

// Integer parsing algorithm adapted for C++ from:
// https://github.com/rust-lang/rust/blob/b935f37/library/core/src/num/mod.rs#L1708
template<Integer Int>
struct IntegerParser {
#define len ((size_t)(end - begin))

    static bool parse(const char* begin, size_t length, Int* result) {
        if (length == 0) return false;
        const char* end = begin + length;

        bool is_positive = true;
        if constexpr(!is_unsigned<Int>) {
            if (*begin == '+' || *begin == '-') {
                is_positive = *begin != '-';
                begin++;
            }
        }

        if (len == 0) return false;

        if (is_positive)
            return Positive::loop(begin, len, result);
        else
            return Negative::loop(begin, len, result);
    }

    template<typename Derived>
    struct Base {
        static bool loop(const char* begin, size_t length, Int* out_result) {
            const char* end = begin + length;

            Int result = 0;

            bool cannot_overflow = len <= sizeof(Int) * 2 - 1;
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

    struct Positive : public Base<Positive> {
        static Int addition(Int a, Int b) {
            return a + b;
        }

        static bool checked_addition(Int a, Int b, Int* result) {
            return __builtin_add_overflow(a, b, result);
        }
    };

    struct Negative : public Base<Negative> {
        static Int addition(Int a, Int b) {
            return a - b;
        }

        static bool checked_addition(Int a, Int b, Int* result) {
            return __builtin_sub_overflow(a, b, result);
        }
    };
#undef len
};

template<Integer Int>
bool int_from_ascii_impl(const char* begin, size_t length, Int* result) {
    return IntegerParser<Int>::parse(begin, length, result);
}

bool u16_from_ascii(const char* begin, size_t length, uint16_t* result) {
    return int_from_ascii_impl<uint16_t>(begin, length, result);
}

bool i64_from_ascii(const char* begin, size_t length, int64_t* result) {
    return int_from_ascii_impl<int64_t>(begin, length, result);
}


}

