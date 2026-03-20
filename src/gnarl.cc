#include "gnarl.h"
#include "error.h"
#include "workspace.h"
#include "input_file.h"
#include "lexer.h"
#include "parser.h"
#include "scope.h"
#include "file_scope.h"
#include "path_io.h"
#include <memory>

namespace gnarl {

std::unique_ptr<Scope> do_run_file(Workspace* workspace, const InputFile* file, Error* error) {
    std::vector<Token> token_buffer = Lexer::lex_to_buffer(file, error);
    if (error->has_error()) return std::unique_ptr<Scope>();

    std::unique_ptr<BaseNode> expr = Parser::parse(token_buffer, error);
    if (error->has_error()) return std::unique_ptr<Scope>();

    std::unique_ptr<Scope> scope = std::make_unique<FileScope>(workspace, file);
    expr->evaluate(scope.get(), error);

    return scope;
}

void run_gnarl(Workspace* workspace, ExitCode* exit_code) {
    Error error;

    PathView make_globals = "//make_globals.gn";
    Path normalized = normalize(make_globals);
    if (pathexists(normalized)) {
        const InputFile* mg = workspace->file_manager()->load_file(make_globals, &error);
        if (mg) {
            std::unique_ptr<Scope> scope = do_run_file(workspace, mg, &error);
            Scope* globals = workspace->globals();

            for (const auto& v : scope->get_values())
                globals->set_value(v.first, Value(v.second)); 
        }
    }

    const InputFile* entry = workspace->file_manager()->load_file("//BUILD.gn", &error);
    if (entry) {
        do_run_file(workspace, entry, &error);
    }


    if (error.has_error()) {
        error.print_to_stdout();
        *exit_code = FAILIURE;
    }
}

}

