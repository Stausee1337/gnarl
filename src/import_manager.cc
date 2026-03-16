
#include "import_manager.h"
#include "path_io.h"
#include "file_scope.h"
#include "input_file.h"
#include <iostream>

namespace gnarl {

const Scope* ImportManager::import_from(PathView path,
                                        const FileScope* file,
                                        const Span& value_span,
                                        const Span& call_span,
                                        Error* error) {
    const InputFile* input_file = file->input_file();
    PathView source_dir = input_file->path().parent();
    Path file_path = resolve_unique(path, source_dir);
    std::cout << file_path.string() << "\n";
    
    return nullptr;
}

}

