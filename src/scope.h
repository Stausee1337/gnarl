
#ifndef GNARL_SCOPE_H_
#define GNARL_SCOPE_H_

#include <string_view>
#include <map>
#include <unordered_map>

#include "value.h"

namespace gnarl {

class Workspace;
class FileScope;
class Template;

class Scope {
public:
    using ValueMap = std::map<std::string_view, Value>;

    template<typename T = void>
    struct AttributeKey {};

    struct MergeOptions {
        bool mark_as_used;
        bool skip_private_variables;
        const char* disallow_clobbering = nullptr;
        std::vector<std::string> exclude_list{};
    };

    Scope(Scope* parent);
    Scope(const Scope* parent);

    Scope(const Scope&) = default;
    Scope& operator=(const Scope&) = default;

    const Scope* parent() const;
    const FileScope* file() const { return m_file; }
    // NOTE: detatch_from_parent must not be used with `FileScope`s
    void detatch_from_parent();

    void merge_into(Scope* scope, MergeOptions options, const Span& error_span, Error* error) const;
    std::unique_ptr<Scope> make_closure() const;

    bool has_value(std::string_view name) const;
    const Value* get_value(std::string_view name, bool counts_as_used = true) const;
    Value* get_value_mutable(std::string_view name);
    void mark_as_used(std::string_view name) const;

    void set_value(std::string_view name, Value&& value);

    ValueMap get_values() const;

    bool equals_current_values(const Scope& other) const;

    const Template* get_template(std::string_view name) const;
    void add_template(std::unique_ptr<Template> templ);

    void add_attribute(const AttributeKey<>* key);

    template<typename T>
        requires(!std::is_same_v<T, void>)
    void set_attribute(const AttributeKey<T>* key, const T& value);

    template<typename T>
    void delete_attribute(const AttributeKey<T>* key);

    template<typename T>
        requires(!std::is_same_v<T, void>)
    const T* query_attribute(const AttributeKey<T>* key) const;

    bool query_attribute(const AttributeKey<>* key) const;

protected:
    const FileScope* m_file = nullptr;

private:
    void delete_attribute(uintptr_t key);
    void set_attribute(uintptr_t key, const void* value);
    const void* query_attribute(uintptr_t key) const;
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

    using TemplateMap = std::unordered_map<std::string_view, const Template*>;
    TemplateMap m_templates;

    using AttributeMap = std::unordered_map<uintptr_t, const void*>;
    AttributeMap m_attrs;
};


template<typename T>
    requires(!std::is_same_v<T, void>)
void Scope::set_attribute(const Scope::AttributeKey<T>* key, const T& value) {
    set_attribute((uintptr_t)key, (const void*)&value);
}

template<typename T>
void Scope::delete_attribute(const Scope::AttributeKey<T>* key) {
    delete_attribute((uintptr_t)key);
}

template<typename T>
    requires(!std::is_same_v<T, void>)
const T* Scope::query_attribute(const AttributeKey<T>* key) const {
    return (const T*)query_attribute((uintptr_t)key);
}

}


#endif // GNARL_SCOPE_H_

