
#ifndef GNARL_SCOPE_H_
#define GNARL_SCOPE_H_

#include <string_view>
#include <map>

#include "value.h"

namespace gnarl {

class Scope final {
public:
    using ValueMap = std::map<std::string_view, Value>;

    Scope() = default;
    Scope(Scope* parent);

    Scope(const Scope&) = default;
    Scope& operator=(const Scope&) = default;

    const Value* get_value(std::string_view name, bool counts_as_used = true) const;
    Value* get_value_mutable(std::string_view name);

    void set_value(std::string_view name, Value&& value);

    ValueMap get_values() const;

    bool equals_current_values(const Scope& other) const;

private:
    struct ValueInfo {
        Value value;
        bool used;
    };
    using ValueInfoMap = std::map<std::string_view, ValueInfo>;

    const Scope* m_parent;
    ValueInfoMap m_values;
};

}


#endif // GNARL_SCOPE_H_

