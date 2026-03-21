
#ifndef GNARL_TEMPLATE_H_
#define GNARL_TEMPLATE_H_

#include <string>
#include <memory>
#include "position.h"

namespace gnarl {

class Scope;
class BlockNode;
class Error;

class Template final {
public:
    Template(std::string name, Scope* scope, Span span, const BlockNode* block);

    const std::string& name() const { return m_name; }
    const Span& span() const { return m_span; }

    void invoke(Scope* scope, const BlockNode* block, const Span& span, Error* error) const;

private:
    std::string m_name;
    Span m_span;
    const BlockNode* m_block;
    std::unique_ptr<const Scope> m_closure_scope;
};

}

#endif // GNARL_TEMPLATE_H_


