#ifndef GNARL_IMPORT_MANAGER_H_
#define GNARL_IMPORT_MANAGER_H_

#include "position.h"

namespace gnarl {

class Error;
class Scope;
class PathView;
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
    FileManager* m_file_manager;
};

}

#endif // GNARL_IMPORT_MANAGER_H_

