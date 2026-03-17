
#include "error.h"
#include "path_io.h"
#include "file_scope.h"
#include "input_file.h"
#include "lexer.h"
#include "parser.h"
#include "file_manager.h"
#include "import_manager.h"
#include <iostream>
#include <memory>

namespace gnarl {

const Scope* ImportManager::import_from(PathView path,
                                        const FileScope* file,
                                        const Span& value_span,
                                        const Span& call_span,
                                        Error* error) {
    const InputFile* input_file = file->input_file();
    PathView current_dir = input_file->path().parent();
    Path file_path = resolve_unique(path, current_dir);
    std::cout << file_path.string() << "\n";

    const InputFile* import_file = m_file_manager->load_file(file_path, error);
    if (error->has_error())
        return nullptr;

    std::vector<Token> token_buffer = Lexer::lex_to_buffer(import_file, error);
    if (error->has_error())
        return nullptr;

    std::unique_ptr<BaseNode> expr = Parser::parse(token_buffer, error);
    if (error->has_error())
        return nullptr;

    std::unique_ptr<Scope> scope = std::make_unique<FileScope>(file->workspace(), import_file);
    expr->evaluate(scope.get(), error);
    
    return scope.release();
}

}

