#include "gnarl.h"
#include "error.h"
#include "workspace.h"
#include "input_file.h"
#include "lexer.h"
#include "parser.h"
#include "scope.h"
#include "file_scope.h"
#include "format.h"

namespace gnarl {

Value do_run_file(Workspace* workspace, const InputFile& file, Error* error) {
    std::vector<Token> token_buffer = Lexer::lex_to_buffer(file, error);
    if (error->has_error()) return Value();

    std::unique_ptr<BaseNode> expr = Parser::parse(token_buffer, error);
    if (error->has_error()) return Value();

    std::unique_ptr<Scope> scope = std::make_unique<FileScope>(workspace);
    return expr->evaluate(scope.get(), error);
}

void run_gnarl(Workspace* workspace, ExitCode* exit_code) {
    Error error;

    const InputFile* entry = workspace->file_manager()->load_file(normalize("//BUILD.gn"), &error);
    if (entry) {
        do_run_file(workspace, *entry, &error);
    }


    if (error.has_error()) {
        error.print_to_stdout();
        *exit_code = FAILIURE;
    }
}

}

