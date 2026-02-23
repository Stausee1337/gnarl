#include <iostream>

#include "input_file.h"
#include "lexer.h"
#include "parser.h"


int main() {
    auto source = R"a(
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
    

    gnarl::Parser::parse_expression(token_buffer, &error);

    return 0;
}

