
#include <memory>
#include <string>

#include "parser.h"
#include "assertions.h"

namespace gnarl {

enum Precedence {
    PREC_NONE = -1,
    PREC_ASSIGNMENT = 1,
    PREC_BOOLEAN_OR = 2,
    PREC_BOOLEAN_AND = 3,

    PREC_EQUALITY = 4,
    PREC_COMPARISON = 5,
    PREC_ADDITION = 6,
    PREC_PREFIX = 7,
    PREC_CALL = 8,
    PREC_DOT = 9,
};

using PrefixFn = std::unique_ptr<BaseNode>(Parser::*)();
using InfixFn = std::unique_ptr<BaseNode>(Parser::*)(std::unique_ptr<BaseNode> lhs);

struct TokInfo {
    PrefixFn prefix;
    InfixFn infix;
    Precedence prec;
};

const TokInfo Parser::expression_table[] = {
    {nullptr, nullptr, PREC_NONE}, // Error
    {nullptr, nullptr, PREC_NONE}, // EOS

    {&Parser::parse_literal, nullptr, PREC_NONE}, // String
    {&Parser::parse_literal, nullptr, PREC_NONE}, // Integer

    {&Parser::parse_identifier_or_call, nullptr, PREC_NONE}, // Identifier
    {nullptr, nullptr, PREC_NONE}, // If
    {nullptr, nullptr, PREC_NONE}, // Else
    {&Parser::parse_literal, nullptr, PREC_NONE}, // True
    {&Parser::parse_literal, nullptr, PREC_NONE}, // False

    {nullptr, &Parser::parse_assign_operator, PREC_ASSIGNMENT}, // Equal
    {nullptr, &Parser::parse_binary_operator, PREC_ADDITION}, // Plus
    {nullptr, &Parser::parse_binary_operator, PREC_ADDITION}, // Minus
    {nullptr, &Parser::parse_assign_operator, PREC_ASSIGNMENT}, // PlusEquals
    {nullptr, &Parser::parse_assign_operator, PREC_ASSIGNMENT}, // MinusEquals
    {nullptr, &Parser::parse_binary_operator, PREC_EQUALITY}, // EqualEqual
    {nullptr, &Parser::parse_binary_operator, PREC_EQUALITY}, // NotEqual
    {nullptr, &Parser::parse_binary_operator, PREC_COMPARISON}, // LessEqual
    {nullptr, &Parser::parse_binary_operator, PREC_COMPARISON}, // GreaterEqual
    {nullptr, &Parser::parse_binary_operator, PREC_COMPARISON}, // LessThan
    {nullptr, &Parser::parse_binary_operator, PREC_COMPARISON}, // GreaterThan
    {nullptr, &Parser::parse_binary_operator, PREC_BOOLEAN_AND}, // BooleanAnd
    {nullptr, &Parser::parse_binary_operator, PREC_BOOLEAN_OR}, // BooleanOr
    {&Parser::parse_not, nullptr, PREC_NONE}, // Bang
    {nullptr, &Parser::parse_dot, PREC_DOT}, // Dot
    {nullptr, nullptr, PREC_NONE}, // Comma

    {&Parser::parse_paren, nullptr, PREC_NONE}, // LParen
    {nullptr, nullptr, PREC_NONE}, // RParen
    {&Parser::parse_list, &Parser::parse_subscript, PREC_CALL}, // LBracket
    {nullptr, nullptr, PREC_NONE}, // RBracket
    {&Parser::parse_block, nullptr, PREC_NONE}, // LCurly
    {nullptr, nullptr, PREC_NONE}, // RCurly

    {nullptr, nullptr, PREC_NONE}, // UnknownOp

    {nullptr, nullptr, PREC_NONE}, // LineComment
    {nullptr, nullptr, PREC_NONE}, // SuffixComment
    {&Parser::parse_block_comment, nullptr, PREC_NONE}, // BlockComment
};

std::unique_ptr<BaseNode> Parser::parse_file() {
    std::unique_ptr<BlockNode> block = std::make_unique<BlockNode>();

    while (!is_eof()) {
        std::unique_ptr<BaseNode> stmt = parse_statement();
        if (error->has_error())
            return std::unique_ptr<BlockNode>();
        if (!stmt)
            break;
        block->append(std::move(stmt));
    }

    if (error->has_error())
        return std::unique_ptr<BlockNode>();

    if (!is_eof()) {
        *error = Error(current().position(), "Unexpected here, exepcted newline");
        return std::unique_ptr<BlockNode>();
    }

    // TODO: assign comments 
    return block;
}

bool is_assignment(const BaseNode* node) {
    if (!node) return false;

    const BinaryOpNode* binary_op = node->as_binary_op();
    if (!binary_op) return false;

    TokenKind kind = binary_op->tok().kind();
    return kind == TokenKind::Equal || kind == TokenKind::PlusEquals || kind == TokenKind::MinusEquals;
}

std::unique_ptr<BaseNode> Parser::parse_statement() {
    if (matches(TokenKind::If))
        return parse_conditional();
    else if (matches(TokenKind::BlockComment))
        return parse_block_comment();

    std::unique_ptr<BaseNode> expression = parse_expression();
    if (expression && (expression->as_function_call() || is_assignment(expression.get())))
        return expression;

    *error = Error(expression.get(), "Expecting assignment or function call");
    return std::unique_ptr<BaseNode>();
}

std::unique_ptr<BaseNode> Parser::parse_conditional() {
    const Token& token = bump();
    if (!expect(TokenKind::LParen, "Expected '(' after 'if'"))
        return std::unique_ptr<BaseNode>();

    std::unique_ptr<BaseNode> condition = parse_expression();
    if (error->has_error())
        return std::unique_ptr<BaseNode>();
    if (is_assignment(condition.get())) {
        *error = Error(condition.get(), "Assignment not allowed in 'if'");
        return std::unique_ptr<BaseNode>();
    }

    if (!expect(TokenKind::RParen, "Expected ')' after condition of 'if'"))
        return std::unique_ptr<BaseNode>();

    if (!matches(TokenKind::LCurly)) {
        *error = Error(current().position(), "Expected '{' to start 'if' block");
        return std::unique_ptr<BaseNode>();
    }

    std::unique_ptr<BlockNode> if_branch = parse_block(BlockNode::Mode::Discard);
    if (error->has_error())
        return std::unique_ptr<BaseNode>();

    std::unique_ptr<ConditionalNode> conditional = std::make_unique<ConditionalNode>(
            token, std::move(condition), std::move(if_branch));

    if (matches(TokenKind::Else)) {
        bump();
        std::unique_ptr<BaseNode> else_branch;

        if (matches(TokenKind::If))
            else_branch = parse_conditional();
        else
            else_branch = parse_block(BlockNode::Mode::Discard);

        if (error->has_error())
            return std::unique_ptr<BaseNode>();
        conditional->set_else_branch(std::move(else_branch));
    }

    return conditional;
}

std::unique_ptr<BaseNode> Parser::parse_expression(int min_prec) {
    if (is_eof())
        return std::unique_ptr<BaseNode>();

    auto lhs = parse_prefix_expression();
    if (error->has_error())
        return std::unique_ptr<BaseNode>();

    TokInfo info = expression_table[(uint)current().kind()];

    while (info.infix != nullptr) {
        if (info.prec < min_prec)
            break;
        lhs = (this->*(info.infix))(std::move(lhs));
        info = expression_table[(uint)current().kind()];
    }

    return lhs;
}

std::unique_ptr<BaseNode> Parser::parse_prefix_expression() {
    const Token& token = current();

    auto prefix = expression_table[(uint)token.kind()].prefix;
    if (prefix == nullptr) {
        *error = Error(token.position(), "Unexpected token '" + std::string(token.value()) + "'");
        return std::unique_ptr<BaseNode>();
    }

    return (this->*(prefix))();
}

std::unique_ptr<BaseNode> Parser::parse_not() {
    const Token& token = bump();
    auto rhs = parse_expression(PREC_PREFIX + 1);
    if (!rhs) {
        if (error->has_error())
            return std::unique_ptr<BaseNode>();
        *error = Error(token.position(), "Exepcted right-hand-side for '!'");
        return std::unique_ptr<BaseNode>();
    }

    return std::make_unique<UnaryOpNode>(token, std::move(rhs));
}

std::unique_ptr<BaseNode> Parser::parse_literal() {
    return std::make_unique<LiteralNode>(bump());
}

std::unique_ptr<BaseNode> Parser::parse_identifier_or_call() {
    const Token& token = bump();
    if (!matches(TokenKind::LParen))
        return std::make_unique<IdentifierNode>(token);

    auto list = parse_comma_seperated_list(TokenKind::RParen, false);
    if (error->has_error())
        return std::unique_ptr<BaseNode>();

    std::unique_ptr<BlockNode> block;
    if (matches(TokenKind::LCurly)) {
        block = parse_block(BlockNode::Mode::Discard);
        if (error->has_error())
            return std::unique_ptr<BaseNode>();
    }

    return std::make_unique<FunctionCallNode>(token, std::move(list), std::move(block));
}

std::unique_ptr<BaseNode> Parser::parse_paren() {
    bump();
    auto expr = parse_expression();
    if (error->has_error())
        return std::unique_ptr<BaseNode>();
    if (!expect(TokenKind::RParen, "Expected ')'"))
        return std::unique_ptr<BaseNode>();
    return expr;
}

std::unique_ptr<BaseNode> Parser::parse_list() {
    auto list = parse_comma_seperated_list(TokenKind::RBracket, true);
    if (error->has_error())
        return std::unique_ptr<BaseNode>();
    return list;
}

std::unique_ptr<BaseNode> Parser::parse_block() {
    auto block = parse_block(BlockNode::Mode::Return);
    if (error->has_error())
        return std::unique_ptr<BaseNode>();
    return block;
}

std::unique_ptr<BaseNode> Parser::parse_block_comment() {
    return std::make_unique<BlockCommentNode>(bump());
}

std::unique_ptr<BaseNode> Parser::parse_binary_operator(std::unique_ptr<BaseNode> lhs) {
    const Token& token = bump();
    auto rhs = parse_expression(expression_table[(uint)token.kind()].prec + 1);
    if (!rhs) {
        if (error->has_error())
            return std::unique_ptr<BaseNode>();
        *error = Error(token.position(), "Exepcted right-hand-side for '" + std::string(token.value()) + "'");
        return std::unique_ptr<BaseNode>();
    }

    return std::make_unique<BinaryOpNode>(token, std::move(lhs), std::move(rhs));
}

std::unique_ptr<BaseNode> Parser::parse_assign_operator(std::unique_ptr<BaseNode> lhs) {
    if (lhs->as_identifier() == nullptr && lhs->as_accessor() == nullptr) {
        *error = Error(lhs.get(), "The left-hand-side of an assignment must be an indentifier, array access or socpe access");
        return std::unique_ptr<BaseNode>();
    }

    const Token& token = bump();
    auto rhs = parse_expression(PREC_ASSIGNMENT + 1);
    if (!rhs) {
        if (error->has_error())
            return std::unique_ptr<BaseNode>();
        *error = Error(token.position(), "Exepcted right-hand-side for assignment");
        return std::unique_ptr<BaseNode>();
    }

    return std::make_unique<BinaryOpNode>(token, std::move(lhs), std::move(rhs));
}

std::unique_ptr<BaseNode> Parser::parse_dot(std::unique_ptr<BaseNode> lhs) {
    bump();

    auto base = lhs->as_identifier();
    if (base == nullptr) {
        // TODO: add "sorry-help-text"
        *error = Error(base, "May only use '.' for identifiers");
        return std::unique_ptr<BaseNode>();
    }

    auto rhs = parse_expression(PREC_DOT);
    if (!rhs || rhs->as_identifier() == nullptr) {
        if (error->has_error())
            return std::unique_ptr<BaseNode>();
        // TODO: add "good-bad-help-text"
        *error = Error(base, "Expected identifier as right-hand-side of '.'");
        return std::unique_ptr<BaseNode>();
    }

    return std::make_unique<AccessorNode>(
        base->tok(),
        std::unique_ptr<IdentifierNode>(static_cast<IdentifierNode*>(rhs.release())));
}

std::unique_ptr<BaseNode> Parser::parse_subscript(std::unique_ptr<BaseNode> lhs) {
    bump();
    auto base = lhs->as_identifier();
    if (base == nullptr) {
        // TODO: add "sorry-help-text"
        *error = Error(base, "May only subscript identifiers");
        return std::unique_ptr<BaseNode>();
    }

    auto rhs = parse_expression();
    if (error->has_error())
        return std::unique_ptr<BaseNode>();
    if (!expect(TokenKind::RBracket, "Expecting ']' after subscript"))
        return std::unique_ptr<BaseNode>();

    return std::make_unique<AccessorNode>(base->tok(), std::move(rhs));
}

std::unique_ptr<BlockNode> Parser::parse_block(BlockNode::Mode node) {
    std::unique_ptr<BlockNode> block = std::make_unique<BlockNode>(bump());
    while (!matches(TokenKind::RCurly)) {

        std::unique_ptr<BaseNode> stmt = parse_statement();
        if (error->has_error())
            return std::unique_ptr<BlockNode>();
        block->append(std::move(stmt));
    }
    block->set_end(bump());

    return block;
}

std::unique_ptr<ListNode> Parser::parse_comma_seperated_list(TokenKind end_token, bool allow_trailing_comma) {
    std::unique_ptr<ListNode> list = std::make_unique<ListNode>(bump());

    bool had_comma = !matches(end_token);
    while (!matches(end_token)) {
        if (!had_comma) {
            *error = Error(current().position(), "Expected comma between items");
            return std::unique_ptr<ListNode>();
        }

        std::unique_ptr<BaseNode> expr = parse_expression(PREC_BOOLEAN_OR);
        if (error->has_error())
            return std::unique_ptr<ListNode>();

        if (is_eof()) {
            *error = Error(list.get(), "Unexpected end of file in list");
            return std::unique_ptr<ListNode>();
        }

        if (expr->as_block_comment())
            had_comma = allow_trailing_comma;
        else
            had_comma = bump_if(TokenKind::Comma);
    }
    if (had_comma && !allow_trailing_comma) {
        *error = Error(current().position(), "Trailing comma");
        return std::unique_ptr<ListNode>();
    }

    list->set_end(bump());

    return list;
}

const Token& Parser::bump() {
    if (cursor >= (ssize_t)buffer.size())
        ABORT("bump() on eof parser");

    const Token& old_token = buffer[cursor];
    TokenKind current_kind;
    do {
        const Token& current = buffer[++cursor];
        switch (current_kind = current.kind()) {
            case TokenKind::LineComment:
                line_commment_tokens.push_back(current);
                break;
            case TokenKind::SuffixComment:
                suffix_commment_tokens.push_back(current);
                break;
            default:
                break;
        }
    } while (current_kind == TokenKind::LineComment || current_kind == TokenKind::SuffixComment);

    // std::cout << "Bump: " << current().value() << "\n";

    return old_token;
}

bool Parser::bump_if(TokenKind kind) {
    bool matched = matches(kind);
    if (matched) bump();
    return matched;
}

bool Parser::matches(TokenKind kind) {
    return current().kind() == kind;
}

bool Parser::expect(TokenKind kind, const char* err_msg) {
    if (!bump_if(kind)) {
        *error = Error(current().position(), std::string(err_msg));
        return false;
    }
    return true;
}

std::unique_ptr<BaseNode> Parser::parse(const std::vector<Token>& buffer, Error* error) {
    Parser p(buffer, error);
    p.bump();
    return p.parse_file();
}

std::unique_ptr<BaseNode> Parser::parse_expression(const std::vector<Token>& buffer, Error* error) {
    Parser p(buffer, error);
    p.bump();
    return p.parse_expression();
}

class StringParser final {
public:
    enum class QuoteKind {
        Single,
        Double,
    };

    StringParser(QuoteKind quote_kind)
        : quote_kind_(quote_kind)
    {}

    void feed(char c);
    bool is_ended() const { return state_ == State::Ended; }

private:
    void normal(char c);
    void escape(char c);

    enum class State {
        Normal,
        Escape,
        Ended,
    };

    QuoteKind quote_kind_;
    State state_ = State::Normal;
    std::string buffer;
};

void StringParser::feed(char c) {
    switch (state_) {
        case State::Normal:
            normal(c);
        case State::Escape:
            escape(c);
        case State::Ended:
            ABORT("call to feed in Ended state");
    }
}

void StringParser::normal(char c) {
    switch (c) {
        case '\\':
            state_ = State::Escape;
            break;
        case '\n':
        case '\r':
            state_ = State::Ended;
            // *err = Err("String literal is unclosed");
            break;
        case '\'':
            if (quote_kind_ == QuoteKind::Single)
                state_ = State::Ended;
            break;
        case '"':
            if (quote_kind_ == QuoteKind::Double)
                state_ = State::Ended;
            break;
        default:
            buffer.push_back(c);
    }
}

void StringParser::escape(char c) {
    state_ = State::Normal;
    switch (c) {
        case '\\': buffer.push_back('\\');
        case '\'':
            if (quote_kind_ == QuoteKind::Single)
                buffer.push_back('\'');
        case '"':
            if (quote_kind_ == QuoteKind::Double)
                buffer.push_back('"');
    }
    buffer.push_back(c);
}

}
