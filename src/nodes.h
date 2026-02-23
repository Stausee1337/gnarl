#ifndef GNARL_NODES_H_
#define GNARL_NODES_H_

#include <memory>
#include <vector>

#include "token.h"

namespace gnarl {

class AccessorNode;
class BinaryOpNode;
class BlockNode;
class FunctionCallNode;
class IdentifierNode;
class ListNode;
class LiteralNode;
class UnaryOpNode;

class BaseNode {
public:
    BaseNode();
    virtual ~BaseNode();

    virtual const AccessorNode* as_accessor() const;
    virtual const BinaryOpNode* as_binary_op() const;
    virtual const BlockNode* as_block() const;
    virtual const FunctionCallNode* as_function_call() const;
    virtual const IdentifierNode* as_identifier() const;
    virtual const ListNode* as_list() const;
    virtual const LiteralNode* as_literal() const;
    virtual const UnaryOpNode* as_unary_op() const;

    BaseNode(const BaseNode&) = delete;
    BaseNode& operator=(const BaseNode&) = delete;

private:


};

class AccessorNode final : public BaseNode {
public:
    AccessorNode(const Token& base, std::unique_ptr<IdentifierNode>&& member)
        : m_base(base),
        m_member(std::move(member))
    {}

    AccessorNode(const Token& base, std::unique_ptr<BaseNode>&& subscript)
        : m_base(base),
        m_subscript(std::move(subscript))
    {}

    AccessorNode(const AccessorNode&) = delete;
    AccessorNode& operator=(const AccessorNode&) = delete;

    const AccessorNode* as_accessor() const override;

    const Token& base() const { return m_base; }
    const IdentifierNode* member() const { return m_member.get(); }
    const BaseNode* subscript() const { return m_subscript.get(); }

private:

    Token m_base;
    std::unique_ptr<IdentifierNode> m_member;
    std::unique_ptr<BaseNode> m_subscript;
};

class BinaryOpNode final : public BaseNode {
public:
    BinaryOpNode(const Token& token, std::unique_ptr<BaseNode>&& lhs, std::unique_ptr<BaseNode>&& rhs)
        : m_tok(token),
        m_lhs(std::move(lhs)),
        m_rhs(std::move(rhs))
    { }

    BinaryOpNode(const BinaryOpNode&) = delete;
    BinaryOpNode& operator=(const BinaryOpNode&) = delete;

    const BinaryOpNode* as_binary_op() const override;

    const Token& tok() const { return m_tok; }
    const BaseNode* lhs() const { return m_lhs.get(); }
    const BaseNode* rhs() const { return m_rhs.get(); }

private:

    Token m_tok;
    std::unique_ptr<BaseNode> m_lhs;
    std::unique_ptr<BaseNode> m_rhs;
};

class BlockNode final : public BaseNode {
public:
    enum class Mode {
        Return, Discard
    };

    BlockNode(const Token& start, const Token& end)
        : m_start(start),
        m_end(end)
    {}

    BlockNode(const BlockNode&) = delete;
    BlockNode& operator=(const BlockNode&) = delete;

    const BlockNode* as_block() const override;

    const Token& end() const { return m_end; }
    const Token& start() const { return m_start; }

private:

    Token m_start;
    Token m_end;
};

class FunctionCallNode final : public BaseNode {
public:
    FunctionCallNode(const Token& function, std::unique_ptr<ListNode>&& args, std::unique_ptr<BlockNode>&& block)
        : m_function(function),
        m_args(std::move(args)),
        m_block(std::move(block))
    {}

    FunctionCallNode(const FunctionCallNode&) = delete;
    FunctionCallNode& operator=(const FunctionCallNode&) = delete;

    const FunctionCallNode* as_function_call() const override;

    const Token& function() const { return m_function; }

private:

    Token m_function;
    std::unique_ptr<ListNode>&& m_args;
    std::unique_ptr<BlockNode>&& m_block;
};

class IdentifierNode final : public BaseNode {
public:
    IdentifierNode(const Token& token)
        : m_tok(token)
    {}

    IdentifierNode(const IdentifierNode&) = delete;
    IdentifierNode& operator=(const IdentifierNode&) = delete;

    const IdentifierNode* as_identifier() const override;

    const Token& tok() const { return m_tok; }

private:

    Token m_tok;
};

class ListNode final : public BaseNode {
public:
    ListNode() = default;

    ListNode(const ListNode&) = delete;
    ListNode& operator=(const ListNode&) = delete;

    const ListNode* as_list() const override;

    const Token& end() const { return m_end; }
    void set_end(const Token& end) { m_end = end; }

    const Token& start() const { return m_start; }
    void set_start(const Token& start) { m_end = start; }

    void append(std::unique_ptr<BaseNode>&& node) {
        m_list.push_back(std::move(node));
    }

private:

    Token m_start;
    Token m_end;

    std::vector<std::unique_ptr<BaseNode>> m_list;
};


class LiteralNode final : public BaseNode {
public:
    LiteralNode(const Token& token)
        : m_tok(token)
    { }

    LiteralNode(const LiteralNode&) = delete;
    LiteralNode& operator=(const LiteralNode&) = delete;

    const LiteralNode* as_literal() const override;

    const Token& tok() const { return m_tok; }
private:

    Token m_tok;
};

class UnaryOpNode final : public BaseNode {
public:
    UnaryOpNode(const Token& token, std::unique_ptr<BaseNode>&& operand)
        : m_tok(token),
        m_operand(std::move(operand))
    { }

    UnaryOpNode(const UnaryOpNode&) = delete;
    UnaryOpNode& operator=(const UnaryOpNode&) = delete;

    const UnaryOpNode* as_unary_op() const override;

    const Token& tok() const { return m_tok; }
    const BaseNode* operand() const { return m_operand.get(); }

private:

    Token m_tok;
    std::unique_ptr<BaseNode> m_operand;
};

}

#endif // GNARL_NODES_H_

