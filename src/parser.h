#ifndef GNARL_PARSER_H_
#define GNARL_PARSER_H_

#include <memory>
#include <vector>

#include "error.h"
#include "nodes.h"
#include "token.h"


namespace gnarl {
struct TokInfo;

class Parser final {
public:
    static std::unique_ptr<BaseNode> parse_expression(const std::vector<Token>& buffer, Error* error);

private:
    Parser(const std::vector<Token>& buffer, Error* error)
        : error(error),
        buffer(buffer)
    {}

    std::unique_ptr<BaseNode> parse_conditional();

    std::unique_ptr<BaseNode> parse_expression(int min_prec = 0);
    std::unique_ptr<BaseNode> parse_prefix_expression();

    std::unique_ptr<BaseNode> parse_not();
    std::unique_ptr<BaseNode> parse_literal();
    std::unique_ptr<BaseNode> parse_identifier_or_call();
    std::unique_ptr<BaseNode> parse_paren();
    std::unique_ptr<BaseNode> parse_list();
    std::unique_ptr<BaseNode> parse_block();
    std::unique_ptr<BlockNode> parse_block(BlockNode::Mode mode);
    std::unique_ptr<BaseNode> parse_block_comment();

    std::unique_ptr<BaseNode> parse_assign_operator(std::unique_ptr<BaseNode> lhs);
    std::unique_ptr<BaseNode> parse_binary_operator(std::unique_ptr<BaseNode> lhs);
    std::unique_ptr<BaseNode> parse_dot(std::unique_ptr<BaseNode> lhs);
    std::unique_ptr<BaseNode> parse_subscript(std::unique_ptr<BaseNode> lhs);

    std::unique_ptr<ListNode> parse_comma_sperated_list(TokenKind end_token, bool allow_trailing_comma);

    static const TokInfo expression_table[];

    const Token& current() const {
        return buffer[cursor];
    }

    const Token& bump();
    bool matches(TokenKind kind);
    bool expect(TokenKind kind, const char* error_msg);

    bool is_eof() const {
        return cursor >= buffer.size();
    }

    size_t cursor = 0;

    Error* error;
    const std::vector<Token>& buffer;
};

}

#endif // GNARL_PARSER_H_

