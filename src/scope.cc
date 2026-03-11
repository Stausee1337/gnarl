
#include "scope.h"

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

}

