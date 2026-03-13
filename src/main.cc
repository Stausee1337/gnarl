#include <iostream>

#include "error.h"
#include "workspace.h"
#include "input_file.h"
#include "lexer.h"
#include "parser.h"
#include "scope.h"
#include "file_scope.h"
#include "format.h"
#include "path_io.h"

gnarl::Value do_run_file(const gnarl::InputFile& file, gnarl::Error* error) {
    std::vector<gnarl::Token> token_buffer = gnarl::Lexer::lex_to_buffer(file, error);
    if (error->has_error()) return gnarl::Value();

    std::unique_ptr<gnarl::BaseNode> expr = gnarl::Parser::parse(token_buffer, error);
    if (error->has_error()) return gnarl::Value();

    gnarl::Workspace workspace(gnarl::Workspace::Options{});
    std::unique_ptr<gnarl::Scope> scope = std::make_unique<gnarl::FileScope>(&workspace);
    return expr->evaluate(scope.get(), error);
}

int main() {
    auto source = R"a(
print(gnarl_version)
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

    gnarl::print("{}\n", gnarl::normalize("//../a/b/c", "/home/MISC/stausee1337"));

    return 0;
}

