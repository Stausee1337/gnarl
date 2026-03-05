#include <iostream>

#include "input_file.h"
#include "lexer.h"
#include "parser.h"
#include "scope.h"


int main() {
    auto source = R"a(
# x = ["apple", "pear", "ivy"] - ["pear"]
# print(x)
# 
# foreach(i, x) {
#     assert(i != "pear", "Pear can't be here")
# }
# 
# os_path = getenv("PATH")
# 
# print(len(os_path))
# print(os_path)
# 
# paths = string_split(os_path, ":")
# print(paths, len(paths))
# chunked = split_list(paths, 3)
# print(chunked)

# print(string_join(":", paths) == os_path)
print(string_split("  a b  "))

# print(string_replace("The fat cat sat", "at", "un"))

)a";

    gnarl::Error error;
    gnarl::InputFile file(source);

    std::vector<gnarl::Token> token_buffer = gnarl::Lexer::lex_to_buffer(file, &error);

    if (error.has_error()) {
        std::cerr << error.message() << "\n"; 
        std::cerr << error.help() << "\n";
        return 1;
    }

    // for (const auto& token : token_buffer) {
    //     printf("%d:%d: %.*s\n",
    //            token.position().lineno(),
    //            token.position().column(),
    //            (int)token.value().size(),
    //            token.value().data());
    // }
    

    auto expr = gnarl::Parser::parse(token_buffer, &error);
    if (error.has_error()) {
        std::cerr << error.message() << "\n"; 
        std::cerr << error.help() << "\n";
        return 1;
    }

    std::unique_ptr<gnarl::Scope> scope = std::make_unique<gnarl::Scope>();

    gnarl::Value v = expr->evaluate(scope.get(), &error);
    if (error.has_error()) {
        std::cerr << error.message() << "\n"; 
        std::cerr << error.help() << "\n";
        return 1;
    }

    return 0;
}

