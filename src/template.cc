
#include "scope.h"
#include "template.h"

namespace gnarl {

Template::Template(std::string name, Scope* scope, Span span, const BlockNode* block) 
    : m_name(name), m_span(span), m_block(block) {
    m_closure_scope = scope->make_closure();
}

void Template::invoke(Scope* scope, const BlockNode* block, const Span& span, Error* error) const {
    // TODO: call tracing
    
    ABORT("not yet implemented");
}

}

