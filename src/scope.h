
#ifndef GNARL_SCOPE_H_
#define GNARL_SCOPE_H_

#include <string_view>
#include <unordered_map>

#include "value.h"

namespace gnarl {

class Scope final {
public:
    Scope() = default;

    const Value* get_value(std::string_view name, bool counts_as_used = true);
    void set_value(std::string_view name, Value&& value);

private:
    struct ValueInfo {
        Value value;
        bool used;
    };

    std::unordered_map<std::string_view, ValueInfo> m_values;
};

}


#endif // GNARL_SCOPE_H_

