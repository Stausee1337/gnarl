
#include <stdio.h>
#include <stdlib.h>

#include "assertions.h"

namespace assertions {

[[noreturn]] void DCheck(const char* condition, const Location& location) {
    fprintf(stderr, "%s:%u: %s: ", location.filename, location.lineno, location.function_name);
    fprintf(stderr, "DCHECK failed: `%s`\n", condition);
    abort();
}

[[noreturn]] void Abort(const char* message, const Location& location) {
    fprintf(stderr, "%s:%u: %s: ", location.filename, location.lineno, location.function_name);
    fprintf(stderr, "Abort: `%s`\n", message);
    abort();
}

}

