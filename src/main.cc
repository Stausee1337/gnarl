#include <iostream>

#include "input_file.h"
#include "lexer.h"


int main() {
    auto source = R"a(
inputs = idl_lexer_parser_files + idl_compiler_files # to be explicit (covered by parsetab)
inputs += "hi"

if (true) {
  if (true) {
    inputs = idl_lexer_parser_files + idl_compiler_files # to be explicit (covered by parsetab)
    inputs += "hi"
  }
}

if (true) {
  if (something) {
    a = b
  } else {  # !is_chromeos
    os_category = current_os
  }
  no_blank_here = true
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

    for (const auto& token : token_buffer) {
        printf("%d:%d: %.*s\n",
               token.position().lineno(),
               token.position().column(),
               (int)token.value().size(),
               token.value().data());
    }

    return 0;
}

