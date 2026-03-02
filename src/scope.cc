
#include "scope.h"

namespace gnarl {

Scope::Scope(Scope* parent) : m_parent(parent)
{}

const Value* Scope::get_value(std::string_view name, bool counts_as_used) const {
    Scope& self = *const_cast<Scope*>(this);
    ValueInfoMap::iterator found_value = self.m_values.find(name);
    if (found_value == self.m_values.end())
        return nullptr;
    ValueInfo& info = found_value->second;
    info.used |= counts_as_used;
    return &info.value;
}

Value* Scope::get_value_mutable(std::string_view name) {
    ValueInfoMap::iterator found_value = m_values.find(name);
    if (found_value == m_values.end())
        return nullptr;
    return &found_value->second.value;
}

Scope::ValueMap Scope::get_values() const {
    ValueMap map;
    for (const auto& p : m_values) {
        map[p.first] = p.second.value;
    }
    return map;
}

bool Scope::equals_current_values(const Scope& other) const {
    if (m_values.size() != other.m_values.size())
        return false;
    for (const auto& p : m_values) {
        // FIXME: counts_as_used may need to be true
        const Value* value = other.get_value(p.first, /*counts_as_used=*/ false);
        if (!value || *value != p.second.value)
            return false;
    }
    return true;
}

}

