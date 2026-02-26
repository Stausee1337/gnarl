
#ifndef GNARL_ASSERTIONS_H_
#define GNARL_ASSERTIONS_H_

#include <stdint.h>

namespace assertions {

#define _IMPL_CHECK(call, condition)    \
    do {                                \
    if ((condition) ? true : false)     \
        [[likely]];                     \
    else                                \
         (call);                        \
    } while (0)

// TODO: only activate if debug build
#define DCHECK(condition) \
    _IMPL_CHECK(::assertions::DCheck(#condition), condition)

#define ABORT(message) \
    ::assertions::Abort(message)

struct Location {

    [[nodiscard]] static Location GetCallerLocation(
            const char* function = __builtin_FUNCTION(),
            const char* filename = __builtin_FILE(),
            uint32_t lineno = __builtin_LINE());

    const char* function_name;
    const char* filename;
    uint32_t lineno;
};

[[noreturn]] void DCheck(const char* string, const Location& location = Location::GetCallerLocation());
[[noreturn]] void Abort(const char* string, const Location& location = Location::GetCallerLocation());

inline
Location Location::GetCallerLocation(const char* function, const char* filename, uint32_t lineno) {
    return Location(function, filename, lineno);
}

}

#endif // GNARL_ASSERTIONS_H_

