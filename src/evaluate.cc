
#include <algorithm>
#include <type_traits>

#include "error.h"
#include "nodes.h"
#include "token.h"
#include "value.h"
#include "scope.h"
#include "assertions.h"

namespace gnarl {

#define CONTEXT Scope* scope, Error* error

#define EVAL2VAL(node, rv, ...) \
    ({                                                                                  \
        Value result = (node).evaluate(scope, error);                                   \
        if (error->has_error())                                                         \
            return (rv);                                                                \
        if (result.kind() == Value::Kind::None) {                                       \
            *error = make_void_value_error((node).get_span() __VA_OPT__(,) __VA_ARGS__);\
            return (rv);                                                                \
        }                                                                               \
        result;                                                                         \
    })


#define LOOKUP(token, rv)                                           \
    ({                                                              \
        const Value* value = scope->get_value(token.value());       \
        if (!value) {                                               \
            *error = Error(token.span(), "Undefined identifier");   \
            return (rv);                                            \
        }                                                           \
        value;                                                      \
    })

#define LOOKUP_MUT(token, rv)                                       \
    ({                                                              \
        Value* value = scope->get_value_mutable(token.value());     \
        if (!value) {                                               \
            *error = Error(token.span(), "Undefined identifier");   \
            return (rv);                                            \
        }                                                           \
        value;                                                      \
    })

Error make_void_value_error(
    const Span& span,
    std::string message = "This does not evaluate to a value",
    std::string help = "I can't do something with nothing") {
    return Error(span, message, help);
}

enum Mutability {
    MUT_MUTABLE,
    MUT_IMMUTABLE
};

template<Mutability M, typename T>
using GenericRef = std::conditional_t<M == MUT_MUTABLE, T&, const T&>;

template<Mutability M, typename T>
using GenericPtr = std::conditional_t<M == MUT_MUTABLE, T*, const T*>;

template<Mutability M, typename T>
using GenericResult = std::conditional_t<M == MUT_MUTABLE, T*, T const**>;

template<Mutability MUT>
void access_list(CONTEXT,
        GenericRef<MUT, std::vector<Value>> list,
        const BaseNode& subscript_node,
        GenericResult<MUT, Value> result) {
    Value subscript_value = EVAL2VAL(subscript_node, (void)0);
    if (!subscript_value.typeck(Value::Kind::Integer, error, subscript_node.get_span()))
        return;
    int64_t subscript = subscript_value.as_integer();
    if (subscript < 0) {
        *error = Error(
            subscript_node.get_span(),
            "Negative array subscript",
            "You gave me " + std::to_string(subscript)
        );
        return;
    }
    if ((size_t)subscript >= list.size()) {
        if (list.empty())
            *error = Error(
                subscript_node.get_span(),
                "Array subscript out of range",
                "You gave me " + std::to_string(subscript) + " but the array has no elements"
            );
        else
            *error = Error(
                subscript_node.get_span(),
                "Array subscript out of range",
                "You gave me " + std::to_string(subscript) + " but I was expecting something from 0 to " + std::to_string(list.size() - 1) + ", inclusive"
            );

        return;
    }

    if constexpr (MUT == MUT_MUTABLE)
        list[subscript] = std::move(*result);
    else
        *result = &list[subscript];
}

template<Mutability MUT>
void access_scope(CONTEXT,
        GenericRef<MUT, Scope> obj,
        const AccessorNode& accessor_node,
        bool is_subscript,
        GenericResult<MUT, Value> result) {

    Span member_span;
    std::string member_name;
    if (is_subscript) {
        const BaseNode& subscript_node = *accessor_node.subscript();
        member_span = subscript_node.get_span();

        Value subscript_value = EVAL2VAL(subscript_node, (void)0);
        if (!subscript_value.typeck(Value::Kind::String, error, member_span))
            return;
        member_name = subscript_value.as_string();
    } else {
        const IdentifierNode* node = accessor_node.member();
        member_name = node->tok().value();
        member_span = node->get_span();
    }

    if constexpr (MUT == MUT_MUTABLE)  {
        obj.set_value(member_name, std::move(*result));
    } else {
        const Value* value = obj.get_value(member_name);
        if (value == nullptr) {
            *error = Error(
                member_span,
                "No value named \"" + member_name + "\" in scope \"" + std::string(accessor_node.base().value()) + "\""
            );
        }
        *result = value;
    }
}

template<Mutability MUT>
void generic_accessor(CONTEXT,
        const AccessorNode& accessor,
        GenericResult<MUT, Value> result) {
    GenericPtr<MUT, Value> value;
    if constexpr (MUT == MUT_MUTABLE)
        value = LOOKUP_MUT(accessor.base(), (void)0);
    else
        value = LOOKUP(accessor.base(), (void)0);

    bool is_subscript = !!accessor.subscript();

    switch (value->kind()) {
        case Value::Kind::List:
            if (is_subscript)
                access_list<MUT>(scope, error, value->as_list(), *accessor.subscript(), result);
            break;
        case Value::Kind::Scope:
            access_scope<MUT>(scope, error, value->as_scope(), accessor, is_subscript, result);
            break;
        default:
            break;
    }

    if (error->has_error())
        return;

    if (is_subscript)
        *error = Error(
            accessor.base().span(),
            "Expecting either a list or a scope for a subscript, got " + std::string(Value::type_name(*value))
        );
    else
        *error = Error(
            accessor.base().span(),
            "Expecting a scope for named access, got " + std::string(Value::type_name(*value))
        );
    return;
}
 
Value AccessorNode::evaluate(CONTEXT) const {
    const Value* result;
    generic_accessor<MUT_IMMUTABLE>(scope, error, *this, &result);
    if (error->has_error())
        return Value();
    Value mutable_copy(*result);
    mutable_copy.set_origin(get_span());
    return mutable_copy;
}

void AccessorNode::evaluate_set(CONTEXT, Value value) {
    generic_accessor<MUT_MUTABLE>(scope, error, *this, &value);
}

bool evaluate_integer_comparisson(Error* error, TokenKind op, const Value& lhs_value, const Value& rhs_value, const Span& cmp_span) {
    bool lhs_valid = lhs_value.kind() == Value::Kind::Integer;
    bool rhs_valid = rhs_value.kind() == Value::Kind::Integer;

    if (!lhs_valid || !rhs_valid) {
        *error = Error(
            cmp_span,
            "Comparison requires two integers", 
            "This operator can only compare two integers"
        );
        return false;
    }

    int64_t lhs = lhs_value.as_integer();
    int64_t rhs = rhs_value.as_integer();

    switch (op) {
        case TokenKind::LessThan:
            return lhs < rhs;
        case TokenKind::LessEqual:
            return lhs <= rhs;
        case TokenKind::GreaterThan:
            return lhs > rhs;
        case TokenKind::GreaterEqual:
            return lhs >= rhs;
        default:
            ABORT("invalid token kind in evaluate_integer_comparisson");
    }
}

void remove_matches_from_list(std::vector<Value>& list, const Value& to_remove, Error* error) {
    switch (to_remove.kind()) {
        case Value::Kind::Boolean:
        case Value::Kind::Integer:
        case Value::Kind::String:
        case Value::Kind::Scope:
        {
            std::vector<Value>::const_iterator iter = std::find(list.begin(), list.end(), to_remove);
            if (iter == list.end()) {
                *error = Error(
                    *to_remove.origin(),
                    "Item not found",
                    "You were trying to remove " + to_remove.display() + " but it wasn't there"
                );
                return;
            }
            list.erase(iter);
        }
        break;
        case Value::Kind::List:
        {
            for (const auto& v : to_remove.as_list()) {
                remove_matches_from_list(list, v, error);
                if (error->has_error())
                    break;
            }
        }
        break;
        case Value::Kind::None:
            break;
    }
}

enum Arithmetic {
    ATH_PLUS,
    ATH_MINUS,
};

template<Arithmetic ATH>
Value evaluate_object_arithmetic(Error* error, const Value& lhs_value, const Value& rhs_value, const BaseNode* op_node, bool assignop = false) {
    Value::Kind kind = Value::Kind::None;

    if (lhs_value.kind() == rhs_value.kind())
        kind = lhs_value.kind();

    if constexpr (ATH == ATH_MINUS)
        kind = kind == Value::Kind::String ? Value::Kind::None : kind;

    if (!(kind == Value::Kind::Integer || kind == Value::Kind::String || kind == Value::Kind::List)) {
        const char* op_name;
        const char* op_symbol;

        if constexpr (ATH == ATH_PLUS) {
            op_name = "add";
            op_symbol = assignop ? "+=" : "+";
        } else {
            op_name = "subtract";
            op_symbol = assignop ? "-=" : "-";
        }

        // FIXME: Add extra clarification on add/remove item if lhs is a list
        *error = Error(
            op_node->get_span(),
            "Incompatible types to " + std::string(op_name),
            "You can't do <" + std::string(Value::type_name(lhs_value)) + "> " + std::string(op_symbol) + " <" + std::string(Value::type_name(rhs_value)) + ">"
        );

        return Value();
    }

    switch (kind) {
        case Value::Kind::Integer:
            if constexpr (ATH == ATH_PLUS)
                return Value(op_node, lhs_value.as_integer() + rhs_value.as_integer());
            else
                return Value(op_node, lhs_value.as_integer() - rhs_value.as_integer());
        case Value::Kind::String:
            return Value(op_node, lhs_value.as_string() + rhs_value.as_string());
        case Value::Kind::List:
        {
            if constexpr (ATH == ATH_PLUS) {
                std::vector<Value> result;
                for (const auto& value : lhs_value.as_list())
                    result.push_back(std::move(value));
                for (const auto& value : rhs_value.as_list())
                    result.push_back(std::move(value));
                return Value(op_node, std::move(result));
            }

            Value mutable_copy(lhs_value);

            remove_matches_from_list(mutable_copy.as_list(), rhs_value, error);
            if (error->has_error())
                return Value();

            mutable_copy.set_origin(op_node->get_span());
            return mutable_copy;
        }
        break;
        default:
            ABORT("invalid constellation in evaluate_object_arithmetic");
    } 
}

Value evaluate_binary_operator(CONTEXT, const BinaryOpNode& op_node) {
    const BaseNode& lhs_node = *op_node.lhs();
    const BaseNode& rhs_node = *op_node.rhs();

#define EMPTY_MSG "Operator requires a value"
    Value lhs_value = EVAL2VAL(lhs_node, Value(), EMPTY_MSG, "The thing on the left does not evaluate to a value");
    Value rhs_value = EVAL2VAL(rhs_node, Value(), EMPTY_MSG, "The thing on the right does not evaluate to a value");
#undef EMPTY_MSG

    TokenKind op = op_node.tok().kind();
    switch (op) {
        case TokenKind::EqualEqual:
            return Value(&op_node, lhs_value == rhs_value);
        case TokenKind::NotEqual:
            return Value(&op_node, lhs_value != rhs_value);
        case TokenKind::LessEqual:
        case TokenKind::GreaterEqual:
        case TokenKind::LessThan:
        case TokenKind::GreaterThan:
            return Value(&op_node, evaluate_integer_comparisson(error, op, lhs_value, rhs_value, op_node.get_span()));
        case TokenKind::Plus:
            return evaluate_object_arithmetic<ATH_PLUS>(error, lhs_value, rhs_value, &op_node);
        case TokenKind::Minus:
            return evaluate_object_arithmetic<ATH_MINUS>(error, lhs_value, rhs_value, &op_node);
        default:
            ABORT("invalid token kind in BinaryOpNode");
    }
}

Value BinaryOpNode::evaluate(CONTEXT) const {
    TokenKind op = m_tok.kind();
    if (!(op == TokenKind::Equal || op == TokenKind::PlusEquals || op == TokenKind::MinusEquals))
        return evaluate_binary_operator(scope, error, *this);

    Value lhs_value;
    if (op == TokenKind::PlusEquals || op == TokenKind::MinusEquals) {
        lhs_value = m_lhs->evaluate(scope, error);
        if (error->has_error())
            return Value();
    }

    Value rhs_value = EVAL2VAL(*m_rhs, Value(), "Operator requires rvalue", "The thing on the right does not evaluate to a value");

    Value value;
    switch (op) {
        case TokenKind::Plus:
            value = evaluate_object_arithmetic<ATH_PLUS>(error, lhs_value, rhs_value, this, /*assignop=*/ true);
            break;
        case TokenKind::Minus:
            value = evaluate_object_arithmetic<ATH_MINUS>(error, lhs_value, rhs_value, this, /*assignop=*/ true);
            break;
        default:
            value = std::move(rhs_value);
    }
    if (error->has_error())
        return Value();

    m_lhs->evaluate_set(scope, error, value);
    return Value();
}

Value BlockNode::evaluate(CONTEXT) const {
    Scope* current_scope;
    std::unique_ptr<Scope> owned_scope;
    if (m_mode == Mode::Return) {
        owned_scope = std::make_unique<Scope>(scope);
        current_scope = owned_scope.get();
    } else
        current_scope = scope;

    evaluate_in_scope(current_scope, error);
    if (error->has_error())
        return Value();

    if (m_mode == Mode::Discard)
        return Value();

    owned_scope->detatch_from_parent();
    return Value(this, std::move(owned_scope));
}

void BlockNode::evaluate_in_scope(CONTEXT) const {
    for (const auto& stmt : m_stmts) {
        stmt->evaluate(scope, error);
        if (error->has_error())
            return;
    }
}

Value BlockCommentNode::evaluate(CONTEXT) const {
    return Value();
}

Value ConditionalNode::evaluate(CONTEXT) const {
    Value condition_value = EVAL2VAL(*m_condition, Value());
    if (condition_value.kind() != Value::Kind::Boolean) {
        *error = Error(
            m_condition->get_span(),
            "Condition does not evaluate to a boolean value",
            "This is a value of type \"" + std::string(Value::type_name(condition_value)) + "\" instead"
        );
        return Value();
    }

    bool condition = condition_value.as_boolean();
    if (condition)
        m_if_branch->evaluate(scope, error);
    else if (m_else_branch)
        m_else_branch->evaluate(scope, error);

    return Value();
}

Value FunctionCallNode::evaluate(CONTEXT) const {
    *error = Error(get_span(), "Function calls aren't implemented");
    return Value();
}

template<Mutability MUT>
void generic_identifier(CONTEXT, const Token& identifier, GenericResult<MUT, Value> result) {
    if constexpr (MUT == MUT_MUTABLE)
        scope->set_value(identifier.value(), std::move(*result));
    else
        *result = LOOKUP(identifier, (void)0);
}

Value IdentifierNode::evaluate(CONTEXT) const {
    const Value* result;
    generic_identifier<MUT_IMMUTABLE>(scope, error, m_tok, &result);
    if (error->has_error())
        return Value();
    Value mutable_copy(*result);
    mutable_copy.set_origin(get_span());
    return mutable_copy;
}

void IdentifierNode::evaluate_set(CONTEXT, Value value) {
    generic_identifier<MUT_MUTABLE>(scope, error, m_tok, &value);
}

Value ListNode::evaluate(CONTEXT) const {
    std::vector<Value> list;
    for (const auto& item : m_items) {
        if (item->as_block_comment()) continue;
        Value value = EVAL2VAL(*item, Value());
        list.push_back(std::move(value));
    }

    return Value(this, std::move(list));
}

Value UnaryOpNode::evaluate(CONTEXT) const {
    DCHECK(m_tok.kind() == TokenKind::Bang);
    return EVAL2VAL(*m_operand, Value());
}

void BaseNode::evaluate_set(CONTEXT, Value value) {
    ABORT("evaluate_set() cannot is not implemented for this node");
}

}

