#include <iostream>

#include "error.h"
#include "input_file.h"
#include "lexer.h"
#include "parser.h"
#include "scope.h"
#include "format.h"

gnarl::Value do_run_file(const gnarl::InputFile& file, gnarl::Error* error) {
    std::vector<gnarl::Token> token_buffer = gnarl::Lexer::lex_to_buffer(file, error);
    if (error->has_error()) return gnarl::Value();

    std::unique_ptr<gnarl::BaseNode> expr = gnarl::Parser::parse(token_buffer, error);
    if (error->has_error()) return gnarl::Value();

    std::unique_ptr<gnarl::Scope> scope = std::make_unique<gnarl::Scope>();
    return expr->evaluate(scope.get(), error);
}

int main() {
    auto source = R"a(
scope = { a = 32
x = 42 }

print(scope)
print("Hello, ${scope.a}!")


)a";

    gnarl::Error error;
    gnarl::InputFile file("BUILD.gn", source);

    do_run_file(file, &error);
    if (error.has_error()) {
        error.print_to_stdout();
        return 1;
    }

    // gnarl::print("{:#>$}\n", "test", 3);

    return 0;
}

