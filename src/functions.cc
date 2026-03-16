
#include <algorithm>
#include <sstream>
#include <string.h>
#include <unordered_map>

#include "error.h"
#include "nodes.h"
#include "value.h"
#include "scope.h"
#include "functions.h"
#include "workspace.h"
#include "file_scope.h"

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
    "zero", "one", "two", "three", "four"
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

bool extract_variables_list_or_star(const Value& raw_value,
                                    Error* error,
                                    std::vector<std::string>* variables_list,
                                    bool* is_wildcard) {
    if (raw_value.kind() != Value::Kind::String && raw_value.kind() != Value::Kind::List)
        return false;

    if (raw_value.kind() == Value::Kind::String) {
        if (raw_value.as_string() != "*") return false;
        *is_wildcard = true;
        return true;
    }

    *is_wildcard = false;

    const std::vector<Value>& list = raw_value.as_list();
    for (const auto& v : list) {
        if (!v.typeck(Value::Kind::String, error))
            return false;
        variables_list->push_back(v.as_string());
    }

    return true;
}

Value builtin_forward_variables_from(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    ARGCK("forward_variables_from", 2, 3);

    const Value& from_scope_value = args[0];
    if (!from_scope_value.typeck(Value::Kind::Scope, error))
        return Value();
    const Scope& from_scope = from_scope_value.as_scope();

    const Value& variables_list_or_star_value = args[1];
    Span variables_list_or_star_span = variables_list_or_star_value.origin() ? *variables_list_or_star_value.origin() : call_span;
    bool is_wildcard = false;
    std::vector<std::string> variables_list;
    if (!extract_variables_list_or_star(variables_list_or_star_value, error, &variables_list, &is_wildcard)) {
        if (error->has_error())
            return Value();
        *error = Error(variables_list_or_star_span,
                       "Not a valid list of variables to copy",
                       "Expecting either the string \"*\" or a list of strings");
        return Value();
    }

    std::vector<std::string> exclude_filter;
    if (args.size() > 2) {
        const Value& variables_to_not_forward_value = args[2];
        if (variables_to_not_forward_value.kind() != Value::Kind::List) {
            Span span = variables_to_not_forward_value.origin() ? *variables_to_not_forward_value.origin() : call_span;
            *error = Error(span,
                           "Not a valid list of variables to exclude",
                           "Expecting a list of strings");
            return Value();
        }
        const std::vector<Value>& list = variables_to_not_forward_value.as_list();
        for (const auto& v : list) {
            if (!v.typeck(Value::Kind::String, error))
                return Value();
            exclude_filter.push_back(v.as_string());
        }
    }

    Scope::ValueMap from_scope_values = from_scope.get_values();
    for (const auto& p : from_scope_values) {
        std::vector<std::string>::const_iterator result = std::find(variables_list.begin(), variables_list.end(), p.first);
        if (!(is_wildcard || result != variables_list.end()))
            continue;

        if (std::find(exclude_filter.begin(), exclude_filter.end(), p.first) != exclude_filter.end())
            continue;

        if (!is_wildcard && scope->has_value(p.first)) {
            size_t idx = result - variables_list.begin();
            const Value& name_value = variables_list_or_star_value.as_list()[idx];
            Span name_span = name_value.origin() ? *name_value.origin() : variables_list_or_star_span;
            *error = Error(name_span,
                           "Clobbering existing value",
                           "The current scope already defines a value \"" + std::string(p.first) + "\".\n"
                           "forward_variables_from() won't clobber existing values. If you want to\n"
                           "merge lists you'll need to do that explicitly.");
            const Value& clobbered_value = p.second;
            if (clobbered_value.origin())
                error->append_suberror(Error(*clobbered_value.origin(), "value being clobbered"));
            return Value();
        }

        scope->set_value(p.first, Value(p.second));
        scope->mark_as_used(p.first);
    }

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

Value builtin_import(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    ARGCK("import", 1);

    const Value& path_value = args[0];
    if (!path_value.typeck(Value::Kind::String, error))
        return Value();
    const std::string& path = path_value.as_string();

    Span span = path_value.origin() ? *path_value.origin() : call_span;

    const FileScope* file = scope->file();
    const Scope* import_scope = file->workspace()->import_manager()->import_from(path, file, span, call_span, error);
    if (error->has_error()) 
        return Value();

    Scope::ValueMap import_scope_values = import_scope->get_values();
    for (const auto& p : import_scope_values) {
        Value our_value;
        if (scope->has_value(p.first) && (our_value = *scope->get_value(p.first, false)) != p.second) {
            *error = Error(call_span,
                           "Value collision",
                           "This import contains \"" + std::string(p.first) + "\"");
            const Value& clobbered_value = p.second;
            if (clobbered_value.origin()) {
                error->append_suberror(Error(*clobbered_value.origin(),
                                             "defined here",
                                             "Which would clobber the one in your current scope"));
                if (our_value.origin())
                    error->append_suberror(
                            Error(*our_value.origin(),
                                  "defined here",
                                  "Executing import should not conflict with anything in the current\n"
                                  "scope unless the values are indentical"));
            }
            return Value(); 
        }

        scope->set_value(p.first, Value(p.second));
        scope->mark_as_used(p.first);
    }


    return Value();
}

Value builtin_len(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    ARGCK("len", 1);

    const Value& value = args[0];
    switch(value.kind()) {
        case Value::Kind::String:
            return Value(nullptr, static_cast<int64_t>(value.as_string().size()));
        case Value::Kind::List:
            return Value(nullptr, static_cast<int64_t>(value.as_list().size()));
        default:
        {
            Span span = value.origin() ? *value.origin() : call_span;
            *error = Error(span,
                    "len() expects a string or a list",
                    "Got " + std::string(Value::type_name(value)) + " instead");
            return Value();
        }
        break;

    }
}

Value builtin_not_needed_impl(Error* error, 
                              const Span& call_span,
                              const Scope& target_scope,
                              const std::vector<Value>& args) {
    if (args.size() < 1) {
        *error = Error(call_span,
                       "Wrong number of arguments",
                       "The first argument is a scope, expecting two or three arguments");

        return Value();
    }

    const Value& variables_list_or_star_value = args[0];
    Span variables_list_or_star_span = variables_list_or_star_value.origin() ? *variables_list_or_star_value.origin() : call_span;
    bool is_wildcard = false;
    std::vector<std::string> variables_list;
    if (!extract_variables_list_or_star(variables_list_or_star_value, error, &variables_list, &is_wildcard)) {
        if (error->has_error())
            return Value();
        *error = Error(variables_list_or_star_span,
                       "Not a valid list of variables",
                       "Expecting either the string \"*\" or a list of strings");
        return Value();
    }

    std::vector<std::string> exclude_filter;
    if (args.size() > 1) {
        const Value& variables_to_ignore_value = args[1];
        if (variables_to_ignore_value.kind() != Value::Kind::List) {
            Span span = variables_to_ignore_value.origin() ? *variables_to_ignore_value.origin() : call_span;
            *error = Error(span,
                           "Not a valid list of variables to exclude",
                           "Expecting a list of strings");
            return Value();
        }
        const std::vector<Value>& list = variables_to_ignore_value.as_list();
        for (const auto& v : list) {
            if (!v.typeck(Value::Kind::String, error))
                return Value();
            exclude_filter.push_back(v.as_string());
        }
    }

    for (const auto& v : variables_list) {
        if (target_scope.has_value(v))
            target_scope.mark_as_used(v);
    }

    return Value();
}

Value builtin_not_needed(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    ARGCK("not_needed", 1, 3);

    if (args[0].kind() == Value::Kind::Scope) {
        const Scope& target_scope = args[0].as_scope();
        std::vector<Value> argscpy(args.begin() + 1, args.end());
        return builtin_not_needed_impl(error, call_span, target_scope, argscpy);
    }
    return builtin_not_needed_impl(error, call_span, *scope, args);
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
    ARGCK("split_list", 2);

    const Value& list_value = args[0];
    if (!list_value.typeck(Value::Kind::List, error))
        return Value();
    const std::vector<Value>& list = list_value.as_list();

    const Value& n_value = args[1];
    if (!n_value.typeck(Value::Kind::Integer, error))
        return Value();
    int64_t n = n_value.as_integer();

    std::vector<Value> result;
    result.resize(n);

    int64_t min_items_per_list = static_cast<int64_t>(list.size()) / n;
    int64_t extra_items = static_cast<int64_t>(list.size()) % n;

    int64_t max_items_per_list = min_items_per_list + 1;
    auto prev_item = list.begin();
    for (int64_t i = 0; i < extra_items; ++i) {
        std::vector<Value> sublist;

        auto curr_item = prev_item + max_items_per_list;
        sublist.assign(prev_item, curr_item);
        prev_item = curr_item;

        result[i] = Value(nullptr, std::move(sublist));
    }

    for (int64_t i = extra_items; i < n; ++i) {
        std::vector<Value> sublist;

        auto curr_item = prev_item + min_items_per_list;
        sublist.assign(prev_item, curr_item);
        prev_item = curr_item;

        result[i] = Value(nullptr, std::move(sublist));
    }

    return Value(nullptr, std::move(result));
}

Value builtin_string_join(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    ARGCK("string_join", 2);

    const Value& delimiter_value = args[0];
    if (!delimiter_value.typeck(Value::Kind::String, error))
        return Value();
    const std::string& delimiter = delimiter_value.as_string();

    const Value& list_value = args[1];
    if (!list_value.typeck(Value::Kind::List, error))
        return Value();
    const std::vector<Value>& list = list_value.as_list();

    std::stringstream stream;
    for (auto iterator = list.begin(); iterator != list.end(); ++iterator) {
        if (iterator != list.begin())
            stream << delimiter;
        if (!iterator->typeck(Value::Kind::String, error))
            return Value();
        stream << iterator->as_string();
    }

    return Value(nullptr, stream.str());
}

Value builtin_string_replace(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    ARGCK("string_replace", 3, 4);

    const Value& str_value = args[0];
    if (!str_value.typeck(Value::Kind::String, error))
        return Value();
    std::string_view str = std::string_view(str_value.as_string());

    const Value& old_value = args[1];
    if (!old_value.typeck(Value::Kind::String, error))
        return Value();
    std::string_view old = std::string_view(old_value.as_string());

    const Value& new_value = args[2];
    if (!new_value.typeck(Value::Kind::String, error))
        return Value();
    const std::string& mew = new_value.as_string();

    uint64_t max_split = -1;
    if (args.size() > 3) {
        const Value& maxsplit_value = args[3];
        if (!maxsplit_value.typeck(Value::Kind::Integer, error))
            return Value();
        int64_t maxsplit = maxsplit_value.as_integer();

        if (maxsplit < 1) {
            Span span = maxsplit_value.origin() ? *maxsplit_value.origin() : call_span;
            *error = Error(span, "Requested number of replacements is not positive");
            return Value();
        }
        max_split = maxsplit;
    }

    if (str.size() <= old.size()) {
        if (str == old) return Value(nullptr, std::string(mew));
        return Value(nullptr, std::string(str));
    }

    const char* begin = str.data();
    const char* str_end = str.data() + str.size();
    const char* iter_end = str.data() + (str.size() - old.size());
    const char* current = begin;

    std::stringstream result;
    uint64_t split_count = 0;
    while (current <= iter_end && split_count < max_split) {
        if (strncmp(current, old.data(), old.size()) == 0) {
            split_count++;
            result << mew;
            current += old.size();
        } else {
            result << *current;
            current++;
        }
    }

    result << std::string_view(current, str_end - current);
    return Value(nullptr, result.str());
}

std::string_view chop_by_delim(std::string_view* string, std::string_view delimiter) {
    if (string->length() <= delimiter.length()) {
        std::string_view result = *string;
        *string = "";
        if (result == delimiter)
            return "";
        return result;
    }

    const char* begin = string->data();
    const char* iter_end = begin + (string->size() - delimiter.size());
    const char* string_end = begin + string->size();

    const char* current = begin;
    while (current <= iter_end) {
        if (strncmp(current, delimiter.data(), delimiter.size()) == 0)
            break;
        current++;
    }

    size_t segment_size = current - begin;

    current += delimiter.size();
    if (current > string_end) current = string_end;

    size_t remainder_size = string_end - current;
    *string = std::string_view(current, remainder_size);

    return std::string_view(begin, segment_size);
}

Value builtin_string_split(Scope* scope, Error* error, const Span& call_span, const std::vector<Value>& args) {
    ARGCK("string_split", 1, 2);

    const Value& string_value = args[0];
    if (!string_value.typeck(Value::Kind::String, error))
        return Value();
    std::string_view string = std::string_view(string_value.as_string());

    // FIXME: gn's string_split edge-behaviour is actually quite different.
    // It behaves differently between default (whitespace) and non-default delimiters
    std::string_view delimiter = " ";
    if (args.size() > 1) {
        const Value& delimiter_value = args[1];
        if (!delimiter_value.typeck(Value::Kind::String, error))
            return Value();
        delimiter = delimiter_value.as_string();
        if (delimiter.empty()) {
            Span span = delimiter_value.origin() ? *delimiter_value.origin() : call_span;
            *error = Error(span, "Seperator argument to string_split() cannot be an empty string");
            return Value();
        }
    }

    std::vector<Value> result;
    while (string.length()) {
        std::string_view segment = chop_by_delim(&string, delimiter);
        result.push_back(Value(nullptr, std::string(segment)));
    }

    return Value(nullptr, std::move(result));
}

Value builtin_defined(Scope* scope, Error* error, const Span& call_span, const ListNode& args) {
    ARGCK("defined", 1);

    const BaseNode* node = args[0];
    if (!(node->as_identifier() || node->as_accessor())) {
        *error = Error(call_span,
                      "Bad thing passed to defined()",
                      "It should be of the from defined(foo), defined(foo.bar) or defined(foo[<string-expression>])");
        return Value();
    }

    if (auto ident = node->as_identifier())
        return Value(nullptr, scope->has_value(ident->tok().value()));

    auto accessor = node->as_accessor();
    IdentifierNode identifier(accessor->base());
    Value node_scope_value = identifier.evaluate(scope, error);
    if (error->has_error())
        return Value();
    if (!node_scope_value.typeck(Value::Kind::Scope, error))
        return Value();

    const Scope& node_scope = node_scope_value.as_scope();
    if (auto member = accessor->member())
        return Value(nullptr, node_scope.has_value(member->tok().value()));

    Value subscript_value = accessor->subscript()->evaluate(scope, error);
    if (error->has_error())
        return Value();
    if (!subscript_value.typeck(Value::Kind::String, error))
        return Value();

    const std::string& subscript = subscript_value.as_string();
    return Value(nullptr, node_scope.has_value(subscript));
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

