
#include <unordered_map>

#include "error.h"
#include "nodes.h"
#include "value.h"
#include "functions.h"

namespace gnarl {

Value builtin_print(Scope* scope, Error* error, const std::vector<Value>& args) {
    std::string result;
    for (auto iterator = args.begin(); iterator != args.end(); ++iterator) {
        if (iterator != args.begin())
            result.push_back(' ');
        result += iterator->stringify();
    }
    result.push_back('\n');

    printf("%s", result.c_str());
    return Value();
}

using FlatFn = Value(Scope* scope, Error* error, const std::vector<Value>& args);
using BlockFn = Value(Scope* scope, Error* error, const std::vector<Value>& args, const BlockNode& block);

struct BuiltinInfo final {
    FlatFn* flat_fn;
    BlockFn* block_fn;
};

using BuiltinMap = std::unordered_map<std::string_view, BuiltinInfo>;

struct BuiltinFunctionContainer final {
    BuiltinMap function_map;

    BuiltinFunctionContainer() {
#define REG_FLAT_BUILTIN(name) \
        function_map[#name] = BuiltinInfo {&builtin_##name, nullptr};

ENUMERATE_FLAT_BUILTINS(REG_FLAT_BUILTIN)

#undef REG_FLAT_BUILTIN
    }
};

const BuiltinInfo* lookup_builtin(const std::string_view& name) {
    static BuiltinFunctionContainer builtins;
    BuiltinMap::const_iterator iterator = builtins.function_map.find(name);
    if (iterator == builtins.function_map.end())
        return nullptr;
    return &iterator->second;
}

Value FunctionCallNode::evaluate(Scope* scope, Error* error) const {
    const BuiltinInfo* builtin = lookup_builtin(m_function.value());
    if (!builtin) {
        *error = Error(m_function.span(), "Unknown function");
        return Value();
    }

    if (builtin->flat_fn && m_block) {
        *error = Error(m_block->get_span(),
                       "Unexpected '{'",
                       "This function call doesn't take a {} block following it, and you\n"
                       "can't have a block that's not connected to something like an if\n"
                       "statement or a target declaration");
        error->append_span(m_function.span());
        return Value();
    }

    if (builtin->block_fn && !m_block) {
        *error = Error(m_function.span(), "This function call requires a block");
        return Value();
    }

    Value args_value = m_args->evaluate(scope, error);
    DCHECK(args_value.kind() == Value::Kind::List);
    const std::vector<Value>& args = args_value.as_list();

    // TODO: track call stack
    if (builtin->flat_fn)
        return builtin->flat_fn(scope, error, args);
    return builtin->block_fn(scope, error, args, *m_block);
}

}

