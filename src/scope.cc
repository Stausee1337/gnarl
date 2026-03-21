
#include <algorithm>

#include "scope.h"
#include "error.h"
#include "template.h"

namespace gnarl {

Scope::Scope(Scope* parent) : m_mutable_parent(parent)
{
    if (parent->m_file)
        m_file = parent->m_file;
}

Scope::Scope(const Scope* parent) : m_const_parent(parent)
{
    if (parent->m_file)
        m_file = parent->m_file;
}

bool Scope::has_value(std::string_view name) const {
    ValueInfoMap::const_iterator found_value = m_values.find(name);
    return found_value != m_values.end();
}

const Value* Scope::get_value(std::string_view name, bool counts_as_used) const {
    Scope& self = *const_cast<Scope*>(this);
    ValueInfoMap::iterator found_value = self.m_values.find(name);
    if (found_value != self.m_values.end()) {
        ValueInfo& info = found_value->second;
        info.used |= counts_as_used;
        return &info.value;
    }
    if (parent())
        return parent()->get_value(name, counts_as_used);

    return nullptr;
}

Value* Scope::get_value_mutable(std::string_view name) {
    ValueInfoMap::iterator found_value = m_values.find(name);
    if (found_value != m_values.end()) {
        ValueInfo& info = found_value->second;
        info.used = true;
        return &info.value;
    }
    if (m_mutable_parent)
        return m_mutable_parent->get_value_mutable(name);
    return nullptr;
}

void Scope::mark_as_used(std::string_view name) const {
    Scope& self = *const_cast<Scope*>(this);
    ValueInfoMap::iterator found_value = self.m_values.find(name);
    DCHECK(found_value != self.m_values.end());
    ValueInfo& info = found_value->second;
    info.used = true;
}

void Scope::set_value(std::string_view name, Value&& value) {
    m_values[name] = ValueInfo { .value = value, .used = false };
}

const Scope* Scope::parent() const {
    if (m_mutable_parent) return m_mutable_parent;
    return m_const_parent;
}

void Scope::detatch_from_parent() {
    DCHECK((void*)m_file != this);
    m_mutable_parent = nullptr;
    m_const_parent = nullptr;
    m_file = nullptr;
}

Scope::ValueMap Scope::get_values() const {
    ValueMap map;
    for (const auto& p : m_values) {
        map[p.first] = p.second.value;
    }
    return map;
}

bool Scope::equals_current_values(const Scope& other) const {
    if (parent())
        return false;

    if (m_values.size() != other.m_values.size())
        return false;

    for (const auto& p : m_values) {
        const Value* value = other.get_value(p.first);
        if (!value || *value != p.second.value)
            return false;
    }
    return true;
}

void Scope::merge_into(Scope* scope, MergeOptions options, const Span& error_span, Error* error) const {
    std::vector<std::string> exclude_list = options.exclude_list;

    std::string clobber_kind(options.disallow_clobbering ? options.disallow_clobbering : "");
    for (const auto& p : m_values) {
        if (options.skip_private_variables && p.first.starts_with("_"))
            continue;

        auto exclude_iter = std::find_if(exclude_list.begin(),
                                         exclude_list.end(),
                                         [p](auto v) { return p.first == v; });

        if (exclude_iter != exclude_list.end())
            continue;

        if (options.disallow_clobbering) {
            Value our_value;
            if (scope->has_value(p.first) && (our_value = *scope->get_value(p.first, false)) != p.second.value) {
                *error = Error(error_span,
                               "Value collision",
                               "This " + clobber_kind + " contains \"" + std::string(p.first) + "\"");
                const Value& clobbered_value = p.second.value;
                if (clobbered_value.origin()) {
                    error->append_suberror(Error(*clobbered_value.origin(),
                                "defined here",
                                "Which would clobber the one in your current scope"));
                    if (our_value.origin())
                        error->append_suberror(
                                Error(*our_value.origin(),
                                    "defined here",
                                    "Executing " + clobber_kind + " should not conflict with anything in the current\n"
                                    "scope unless the values are indentical"));
                }
                return; 
            }
        }

        scope->set_value(p.first, Value(p.second.value));
        if (options.mark_as_used)
            scope->mark_as_used(p.first);
    }

    for (const auto& p : m_templates) {
        if (options.skip_private_variables && p.first.starts_with("_"))
            continue;

        auto exclude_iter = std::find_if(exclude_list.begin(),
                                         exclude_list.end(),
                                         [p](auto v) { return p.first == v; });

        if (exclude_iter != exclude_list.end())
            continue;

        if (options.disallow_clobbering) {
            const Template* our_template;
            if ((our_template = scope->get_template(p.first)) != nullptr) {
                *error = Error(error_span,
                               "Template collision",
                               "This " + clobber_kind + " contains a template \"" + std::string(p.first) + "\"");
                const Template* collided_template = p.second;
                error->append_suberror(Error(collided_template->span(),
                            "defined here",
                            "Which would clobber the one in your current scope"));
                error->append_suberror(
                        Error(our_template->span(),
                            "defined here",
                            "Executing " + clobber_kind + " should not conflict with anything in the current\n"
                            "scope"));
                return; 
            }
        }

        scope->m_templates.insert(std::pair(std::string_view(p.second->name()), p.second));
    }
}

std::unique_ptr<Scope> Scope::make_closure() const {
    std::unique_ptr<Scope> closured;

    if (m_const_parent)
        closured = std::make_unique<Scope>(m_const_parent);
    else if (m_mutable_parent)
        closured = m_mutable_parent->make_closure();
    else
        ABORT("parentless scope in make_closure");

    Error error;
    merge_into(closured.get(), MergeOptions{}, Span(), &error);
    DCHECK(!error.has_error());

    return closured;
}

const Template* Scope::get_template(std::string_view name) const {
    TemplateMap::const_iterator iter = m_templates.find(name);
    if (iter == m_templates.end())
        return nullptr;
    return iter->second;
}

void Scope::add_template(std::unique_ptr<Template> templ) {
    m_templates.insert(std::pair(templ->name(), templ.release()));
}

void Scope::add_attribute(const Scope::AttributeKey<>* key) {
    set_attribute((uintptr_t)key, (const void*)1);
}

bool Scope::query_attribute(const Scope::AttributeKey<>* key) const {
    const void* p = query_attribute((uintptr_t)key);
    return p != nullptr;
}

void Scope::delete_attribute(uintptr_t key) {
    AttributeMap::const_iterator iter = m_attrs.find(key);
    if (iter == m_attrs.end())
        return;
    m_attrs.erase(iter);
}

void Scope::set_attribute(uintptr_t key, const void* value) {
    m_attrs.insert(std::pair(key, value));
}

const void* Scope::query_attribute(uintptr_t key) const {
    AttributeMap::const_iterator iter = m_attrs.find(key);
    if (iter != m_attrs.end()) return iter->second;
    if (!parent()) return nullptr;
    return parent()->query_attribute(key);
}

}

