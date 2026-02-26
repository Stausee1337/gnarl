
#ifndef GNARL_SCOPE_H_
#define GNARL_SCOPE_H_

#include <string_view>
#include <unordered_map>

#include "value.h"

namespace gnarl {

class Scope final {
public:
    Scope() = default;
    Scope(Scope* parent);

    Scope(const Scope&) = default;
    Scope& operator=(const Scope&) = default;

    const Value* get_value(std::string_view name, bool counts_as_used = true) const;
    Value* get_value_mutable(std::string_view name);

    void set_value(std::string_view name, Value&& value);

    bool operator==(const Scope& other) const;

private:
    struct ValueInfo {
        Value value;
        bool used;
    };

    std::unordered_map<std::string_view, ValueInfo> m_values;
};

}


#endif // GNARL_SCOPE_H_

