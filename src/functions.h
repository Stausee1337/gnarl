
#ifndef GNARL_FUNCTIONS_H_
#define GNARL_FUNCTIONS_H_

#include <vector>

namespace gnarl {

class Value;
class Scope;
class Error;

#define DECL_FLAT_BUILTIN(name) \
    Value builtin_##name(Scope* scope, Error* error, const std::vector<Value>& args);

#define ENUMERATE_FLAT_BUILTINS(X) \
    X(print)

ENUMERATE_FLAT_BUILTINS(DECL_FLAT_BUILTIN)

#undef DECL_FLAT_BUILTIN
}

#endif // GNARL_FUNCTIONS_H_

