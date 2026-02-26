
#include "value.h"
#include "scope.h"
#include <memory>

namespace gnarl {

Value::~Value() {
    dispose();
}

Value::Value(const Value& other) {
    copy(other);
}

Value& Value::operator=(const Value& other) {
    dispose();
    return copy(other);
}

Value::Value(Value&& other) {
    move(std::move(other));
}

Value& Value::operator=(Value&& other) {
    return move(std::move(other));
}

bool Value::operator==(const Value& other) const {
    if (m_kind != other.m_kind)
        return false;
    switch (m_kind) {
        case Kind::None:
            return true;
        case Kind::Boolean:
            return m_boolean == other.m_boolean;
        case Kind::Integer:
            return m_integer == other.m_integer;
        case Kind::String:
            return m_string == other.m_string;
        case Kind::List:
        {
            if (m_list.size() != other.m_list.size())
                return false;
            return std::equal(m_list.begin(), m_list.end(), other.m_list.begin());
        }
        case Kind::Scope:
            return *m_scope == *other.m_scope;
    }
}

const char* Value::type_name(const Value& value) {
    switch (value.m_kind) {
        case Kind::None:
            return "void";
        case Kind::Boolean:
            return "boolean";
        case Kind::Integer:
            return "integer";
        case Kind::String:
            return "string";
        case Kind::List:
            return "list";
        case Kind::Scope:
            return "scope";
    }
}

void Value::dispose() {
    switch (m_kind) {
        case Kind::String:
            m_string.~basic_string();
            break;
        case Kind::List:
            m_list.~vector();
            break;
        case Kind::Scope:
            m_scope.~unique_ptr();
            break;
        default:
            break;
    }
    m_kind = Kind::None;
}

Value& Value::copy(const Value& other) {
    switch (other.m_kind) {
        case Kind::None:
            break;
        case Kind::Boolean:
            m_boolean = other.m_boolean;
            break;
        case Kind::Integer:
            m_integer = other.m_integer;
            break;
        case Kind::String:
            new (&m_string) std::string(other.m_string);
            break;
        case Kind::List:
            new (&m_list) std::vector<Value>(other.m_list);
            break;
        case Kind::Scope:
            new (&m_scope) std::unique_ptr<Scope>(std::make_unique<Scope>(*other.m_scope.get()));
            break;
    }
    m_kind = other.m_kind;
    return *this;
}

Value& Value::move(Value&& other) {
    switch (other.m_kind) {
        case Kind::None:
            break;
        case Kind::Boolean:
            m_boolean = other.m_boolean;
            break;
        case Kind::Integer:
            m_integer = other.m_integer;
            break;
        case Kind::String:
            new (&m_string) std::string(std::move(other.m_string));
            break;
        case Kind::List:
            new (&m_list) std::vector<Value>(std::move(other.m_list));
            break;
        case Kind::Scope:
            new (&m_scope) std::unique_ptr<Scope>(std::move(other.m_scope));
            break;
    }

    m_kind = other.m_kind;
    other.m_kind = Value::Kind::None;
    return *this;
}

}

