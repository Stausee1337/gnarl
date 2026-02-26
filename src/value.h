
#ifndef GNARL_VALUE_H_
#define GNARL_VALUE_H_

#include <optional>
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "position.h"

namespace gnarl {

class Error;
class Scope;

class Value final {
public:
    enum class Kind {
        None,
        Boolean,
        Integer,
        String,
        List,
        Scope
    };

    Value() : m_kind(Kind::None) {}
    Value(bool boolean) : m_kind(Kind::Boolean), m_boolean(boolean) {}
    Value(int64_t integer) : m_kind(Kind::Integer), m_integer(integer) {}
    Value(std::string&& string) : m_kind(Kind::String), m_string(std::move(string)) {}
    Value(std::vector<Value>&& list) : m_kind(Kind::List), m_list(std::move(list)) {}
    Value(std::unique_ptr<Scope>&& scope) : m_kind(Kind::Scope), m_scope(std::move(scope)) {}

    ~Value();

    Value(const Value&);
    Value& operator=(const Value&);

    Value(Value&&);
    Value& operator=(Value&&);

    const Span* origin() const;
    void set_origin(const Span&); 

    Kind kind() const { return m_kind; }

    bool as_boolean() const { return m_boolean; }
    int64_t as_integer() const { return m_integer; }
    const std::string& as_string() const { return m_string; }

    std::vector<Value>& as_list() { return m_list; }
    const std::vector<Value>& as_list() const{ return m_list; }

    Scope& as_scope() { return *m_scope; }
    const Scope& as_scope() const { return *m_scope; }

    bool typeck(Kind kind, Error* error, Span position = Span());
    std::string display() const;

    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }


    static const char* type_name(const Value&);

private:
    void dispose();
    Value& copy(const Value&);
    Value& move(Value&&);

    Kind m_kind;

    union {
        bool                    m_boolean;
        int64_t                 m_integer;
        std::string             m_string;
        std::vector<Value>      m_list;
        std::unique_ptr<Scope>  m_scope;
    };

    std::optional<Span> m_origin;
};

}

#endif // GNARL_VALUE_H_

