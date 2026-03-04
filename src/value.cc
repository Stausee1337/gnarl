
#include <cstring>
#include <memory>
#include <sstream>
#include "error.h"
#include "value.h"
#include "scope.h"

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
            return as_boolean() == other.as_boolean();
        case Kind::Integer:
            return as_integer() == other.as_integer();
        case Kind::String:
            return as_string() == other.as_string();
        case Kind::List:
        {
            if (m_list.size() != other.m_list.size())
                return false;
            return std::equal(m_list.begin(), m_list.end(), other.m_list.begin());
        }
        case Kind::Scope:
            return m_scope->equals_current_values(other.as_scope());
    }
}

bool Value::typeck(Kind kind, Error* error, Span span) const {
    if (m_kind == kind)
        return true;

    if (!span.file() && m_origin.has_value())
        span = m_origin.value();

    *error = Error(
        span,
        "This is not a " + std::string(type_name(kind)),
        "Instead I see a " + std::string(type_name(*this)) + " = " + display());
    return false;
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
    m_origin = other.m_origin;
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
    m_origin = other.m_origin;
    other.m_origin = std::nullopt;
    return *this;
}

std::string recursive_display(const std::vector<Value>& list) {
    std::string s;
    s.push_back('[');

    for (auto iterator = list.begin(); iterator != list.end(); ++iterator) {
        if (iterator != list.begin())
            s += ", ";
        s += iterator->display();
    }
    s.push_back(']');

    return s;
}

std::string recursive_display(const Scope& scope, uint level) {
    auto values = scope.get_values();
    if (values.size() == 0)
        return "{ }";

    size_t indentation_length = (level + 1) * 2;
    char* indentation = new char[indentation_length + 1];
    memset(indentation, ' ', indentation_length);
    indentation[indentation_length] = '\x00';

    std::stringstream stream;
    stream << "{\n";

    for (auto iterator = values.begin(); iterator != values.end(); ++iterator) {
        stream << indentation;
        stream << iterator->first;
        stream << " = ";
        stream << iterator->second.display(level+1);
        stream << "\n";
    }


    stream << indentation + 2;
    stream << '}';

    // FIXME: use static memory for indentation buffer
    // up to reasonlable level's (e.g. 8)
    delete[] indentation;

    return stream.str();
}

std::string quoted(const std::string& string) {
    std::string result;
    result.push_back('"');

    for (char c : string) {
        if (c == '$' || c == '\\' || c == '"')
            result.push_back('\\');
        result.push_back(c);
    }

    result.push_back('"');
    return result;
}

std::string Value::display(uint level) const {
    switch (m_kind) {
        case Kind::None:
            return "<void>";
        case Kind::Boolean:
            return m_boolean ? "true" : "false";
        case Kind::Integer:
            return std::to_string(m_integer);
        case Kind::String:
            return quoted(m_string);
        case Kind::List:
            return recursive_display(m_list);
        case Kind::Scope:
            return recursive_display(*m_scope.get(), level);
    }
}

std::string Value::stringify() const {
    if (m_kind == Kind::String)
        return as_string();
    return display();
}

const char* Value::type_name(const Value& value) {
    return type_name(value.m_kind);
}

const char* Value::type_name(Kind kind) {
    switch (kind) {
        case Kind::None:
            return "none";
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

}

