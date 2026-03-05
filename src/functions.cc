
#include <unordered_map>

#include "error.h"
#include "nodes.h"
#include "value.h"
#include "scope.h"
#include "functions.h"

namespace gnarl {

#define ARGCK(name, ...)                                                \
    do {                                                                \
        if (!in_range(args.size(), __VA_ARGS__)) {                      \
            *error = Error(call_span,                                   \
                           "Wrong number of arguments to " name "()",   \
                           expected_argcount(name, __VA_ARGS__));       \
            return Value();                                             \
        }                                                               \
    } while (0)

#define NODECK(node, kind, message)         \
    ({                                      \
        if (!node->as_##kind()) {           \
            *error = Error(node, message);  \
            return Value();                 \
        }                                   \
        node->as_##kind();                  \
    })

bool in_range(size_t target, size_t exactly) { return target == exactly; }
bool in_range(size_t target, size_t min, size_t max) {
    return target >= min && target <= max;
}

constexpr const char* count2str[] = {
    "zero", "one", "two", "three"
};

std::string expected_argcount(const char* name, size_t exactly) {
    return std::string(name) + "() takes exactly " + std::string(count2str[exactly]) + "arguments";
}

std::string expected_argcount(const char* name, size_t min, size_t max) {
    if (max - min == 1)
        return std::string(name) 
                + "() takes "
                + std::string(count2str[min])
                + " or "
                + std::string(count2str[max])
                + " arguments";
    return std::string(name) 
            + "() takes between "
            + std::string(count2str[min])
            + " and "
            + std::string(count2str[max])
            + " arguments";
}

Value builtin_assert(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    ARGCK("assert", 1, 2);
    const Value& condition_value = args[0];
    if (!condition_value.typeck(Value::Kind::Boolean, error))
        return Value();
    bool condition = condition_value.as_boolean();

    std::string message;
    if (args.size() > 1) {
        const Value& message_value = args[1];
        if (!message_value.typeck(Value::Kind::String, error))
            return Value();
        message = message_value.as_string();
    }

    if (condition) return Value();

    *error = Error(call_span, "Assertion failed", message);
    return Value();
}

Value builtin_forward_variables_from(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {

    return Value();
}

Value builtin_getenv(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    ARGCK("getenv", 1);

    const Value& key_value = args[0];
    if (!key_value.typeck(Value::Kind::String, error))
        return Value();
    const std::string& key = key_value.as_string();

    // FIXME: lookup based on "opposite" case as well
    std::string result;
    const char* variable = std::getenv(key.c_str());
    if (variable != nullptr)
        result = std::string(variable);

    return Value(nullptr, std::move(result));
}

Value builtin_len(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    ARGCK("len", 1);

    const Value& value = args[0];
    switch(value.kind()) {
        case Value::Kind::String:
            return Value(nullptr, (int64_t)value.as_string().size());
        case Value::Kind::List:
            return Value(nullptr, (int64_t)value.as_list().size());
        default:
        {
            Span span = value.origin() ? *value.origin() : Span();
            *error = Error(span,
                    "len() expects a string or a list",
                    "Got " + std::string(Value::type_name(value)) + " instead");
            return Value();
        }
        break;

    }
}

Value builtin_not_needed(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    return Value();
}

Value builtin_print(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
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

Value builtin_split_list(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    return Value();
}

Value builtin_string_join(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    return Value();
}

Value builtin_string_replace(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    return Value();
}

Value builtin_string_split(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    return Value();
}

Value builtin_defined(Scope* scope, Error* error, const Span& call_span, const ListNode& args) {
    return Value();
}

Value builtin_foreach(Scope* scope,
                      Error* error,
                      const Span& call_span,
                      const ListNode& args,
                      const BlockNode& block) {
    ARGCK("foreach", 2);
    auto identifier = NODECK(args[0], identifier, "Expected identifier for the loop var");

    Value list_value = args.evaluate(scope, error, 1);
    if (error->has_error())
        return Value();
    if (!list_value.typeck(Value::Kind::List, error))
        return Value();

    const std::vector<Value>& list = list_value.as_list();
    for (const auto& v : list) {
        scope->set_value(identifier->tok().value(), Value(v));
        block.evaluate(scope, error);
        if (error->has_error())
            return Value();
    }
    return Value();
}

#define CONTEXT Scope* scope, Error* error, const Span& call

using FlatFunc = Value(CONTEXT, const std::vector<Value>& args);
using FlatMacro = Value(CONTEXT, const ListNode& args);
using BlockFunc = Value(CONTEXT, const std::vector<Value>& args, const BlockNode& block);
using BlockMacro = Value(CONTEXT, const ListNode& args, const BlockNode& block);

#undef CONTEXT

struct BuiltinInfo final {
    enum Linkage {
        func  = 0b00,
        macro = 0b01,
        flat  = 0b00,
        block = 0b10,
    };

    bool is_block() const { return link & block; }
    bool is_macro() const { return link & macro; }
    
    Linkage link;
    union {
        FlatFunc*   flatfunc;
        FlatMacro*  flatmacro;
        BlockFunc*  blockfunc;
        BlockMacro* blockmacro;
    };
};


struct BuiltinFunctionContainer final {
    using Info = BuiltinInfo;
    using Map = std::unordered_map<std::string_view, Info>;

    BuiltinFunctionContainer() {
#define REG_LINKAGE(name, fob, fom) \
    map[#name] = BuiltinInfo { .link = (Info::Linkage)(Info::fob | Info::fom), .fob##fom = &builtin_##name };

#define REG_FLAT_FUNC(name)   REG_LINKAGE(name, flat,  func)
#define REG_FLAT_MACRO(name)  REG_LINKAGE(name, flat,  macro)
#define REG_BLOCK_FUNC(name)  REG_LINKAGE(name, block, func)
#define REG_BLOCK_MACRO(name) REG_LINKAGE(name, block, macro)

BUILTIN_LIST(REG_FLAT_FUNC, REG_FLAT_MACRO, REG_BLOCK_FUNC, REG_BLOCK_MACRO)

#undef REG_FLAT_FUNC
#undef REG_FLAT_MACRO
#undef REG_BLOCK_FUNC
#undef REG_BLOCK_MACRO
#undef REG_LINKAGE
    }

    Map map;
};

const BuiltinInfo* lookup_builtin(const std::string_view& name) {
    static BuiltinFunctionContainer builtins;
    BuiltinFunctionContainer::Map::const_iterator iterator = builtins.map.find(name);
    if (iterator == builtins.map.end())
        return nullptr;
    return &iterator->second;
}

Value FunctionCallNode::evaluate(Scope* scope, Error* error) const {
    const BuiltinInfo* builtin = lookup_builtin(m_function.value());
    if (!builtin) {
        *error = Error(m_function.span(), "Unknown function");
        return Value();
    }

    if (!builtin->is_block() && m_block) {
        *error = Error(m_block->get_span(),
                       "Unexpected '{'",
                       "This function call doesn't take a {} block following it, and you\n"
                       "can't have a block that's not connected to something like an if\n"
                       "statement or a target declaration");
        error->append_span(m_function.span());
        return Value();
    }

    if (builtin->is_block() && !m_block) {
        *error = Error(m_function.span(), "This function call requires a block");
        return Value();
    }

    Span call_span = get_span();

#define MK_FLAT_OR_BLOCK_CALL(func_or_macro)                                                \
    if (builtin->is_block())                                                                \
        result = builtin->block##func_or_macro(scope, error, call_span, args, *m_block);    \
    else                                                                                    \
        result = builtin->flat##func_or_macro(scope, error, call_span, args);

    Value result;

    if (builtin->is_macro()) {
        const ListNode& args = *m_args;
        MK_FLAT_OR_BLOCK_CALL(macro);
    } else {
        Value args_value = m_args->evaluate(scope, error);
        if (error->has_error())
            return Value();
        DCHECK(args_value.kind() == Value::Kind::List);
        const std::vector<Value>& args = args_value.as_list();
        MK_FLAT_OR_BLOCK_CALL(func);
    }

    if (result.kind() != Value::Kind::None)
        result.set_origin(this->get_span());
    return result;

#undef MK_FLAT_OR_BLOCK_CALL
}

}

