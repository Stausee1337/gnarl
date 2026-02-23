
#include "nodes.h"

namespace gnarl {

BaseNode::BaseNode() = default;
BaseNode::~BaseNode() = default;

const AccessorNode* AccessorNode::as_accessor() const {
    return this;
}
Span AccessorNode::get_span() const {
    if (m_member)
        return Span(m_base.position(), m_member->get_span().end());
    return Span(m_base.position(), m_subscript->get_span().end());
}

const BinaryOpNode* BinaryOpNode::as_binary_op() const { return this; }
Span BinaryOpNode::get_span() const {
    return Span(m_lhs->get_span().start(), m_rhs->get_span().end());
}

const BlockNode* BlockNode::as_block() const { return this; }
Span BlockNode::get_span() const {
    if (m_start.kind() != TokenKind::Error && m_end.kind() != TokenKind::Error)
        return Span(m_start.position(), m_end.position());
    if (!m_stmts.empty())
        return Span(m_stmts.front()->get_span().start(), m_stmts.back()->get_span().end());
    return Span();
}

const BlockCommentNode* BlockCommentNode::as_block_comment() const { return this; }
Span BlockCommentNode::get_span() const {
    return m_tok.span();
}

const FunctionCallNode* FunctionCallNode::as_function_call() const { return this; }
Span FunctionCallNode::get_span() const {
    return Span(m_function.position(), m_block ? m_block->get_span().end() : m_args->get_span().end());
}

const IdentifierNode* IdentifierNode::as_identifier() const { return this; }
Span IdentifierNode::get_span() const {
    return m_tok.span();
}

const ListNode* ListNode::as_list() const { return this; }
Span ListNode::get_span() const {
    return Span(m_start.position(), m_end.position());
}

const LiteralNode* LiteralNode::as_literal() const { return this; }
Span LiteralNode::get_span() const {
    return m_tok.span();
}

const ConditionalNode* ConditionalNode::as_conditional() const { return this; }
Span ConditionalNode::get_span() const {
    return Span(m_tok.position(), m_else_branch ? m_else_branch->get_span().end() : m_if_branch->get_span().end());
}

const UnaryOpNode* UnaryOpNode::as_unary_op() const { return this; }
Span UnaryOpNode::get_span() const {
    return Span(m_tok.position(), m_operand->get_span().end());
}

const AccessorNode* BaseNode::as_accessor() const { return nullptr; }
const BinaryOpNode* BaseNode::as_binary_op() const { return nullptr; }
const BlockNode* BaseNode::as_block() const { return nullptr; }
const BlockCommentNode* BaseNode::as_block_comment() const { return nullptr; }
const FunctionCallNode* BaseNode::as_function_call() const { return nullptr; }
const IdentifierNode* BaseNode::as_identifier() const { return nullptr; }
const ListNode* BaseNode::as_list() const { return nullptr; }
const LiteralNode* BaseNode::as_literal() const { return nullptr; }
const ConditionalNode* BaseNode::as_conditional() const { return nullptr; }
const UnaryOpNode* BaseNode::as_unary_op() const { return nullptr; }


}

