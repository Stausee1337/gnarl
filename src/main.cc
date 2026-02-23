#include <iostream>

#include "input_file.h"
#include "lexer.h"
#include "parser.h"


int main() {
    auto source = R"a(

if (x == 12) {

}
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

    return 0;
}

