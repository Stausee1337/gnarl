
#ifndef GNARL_SCOPE_H_
#define GNARL_SCOPE_H_

#include <string_view>
#include <map>

#include "value.h"

namespace gnarl {

class Workspace;
class FileScope;

class Scope {
public:
    using ValueMap = std::map<std::string_view, Value>;

    Scope(Scope* parent);
    Scope(const Scope* parent);

    Scope(const Scope&) = default;
    Scope& operator=(const Scope&) = default;

    const Scope* parent() const;
    const FileScope* file() const { return m_file; }
    // NOTE: detatch_from_parent must not be used with `FileScope`s
    void detatch_from_parent();

    bool has_value(std::string_view name) const;
    const Value* get_value(std::string_view name, bool counts_as_used = true) const;
    Value* get_value_mutable(std::string_view name);
    void mark_as_used(std::string_view name) const;

    void set_value(std::string_view name, Value&& value);

    ValueMap get_values() const;

    bool equals_current_values(const Scope& other) const;

protected:
    const FileScope* m_file = nullptr;

private:
    friend class Workspace;

    Scope() = default;

    struct ValueInfo {
        Value value;
        bool used;
    };
    using ValueInfoMap = std::map<std::string_view, ValueInfo>;

    Scope* m_mutable_parent = nullptr;
    const Scope* m_const_parent = nullptr;

    ValueInfoMap m_values;
};

}


#endif // GNARL_SCOPE_H_

