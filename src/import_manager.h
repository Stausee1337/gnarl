#ifndef GNARL_IMPORT_MANAGER_H_
#define GNARL_IMPORT_MANAGER_H_

#include "position.h"
#include "path_io.h"
#include <unordered_map>

namespace gnarl {

class Error;
class Scope;
class FileScope;
class FileManager;

class ImportManager final {
public:
    ImportManager(FileManager* file_manager)
        : m_file_manager(file_manager)
    {}

    const Scope* import_from(PathView path,
                             const FileScope* file,
                             const Span& value_span,
                             const Span& call_span,
                             Error* error);

private:
    using ImportCache = std::unordered_map<PathView, const FileScope*>;
    FileManager* m_file_manager;
    ImportCache m_import_cache;
};

}

#endif // GNARL_IMPORT_MANAGER_H_

