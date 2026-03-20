
#ifndef GNARL_WORKSPACE_H_
#define GNARL_WORKSPACE_H_

#include <string>
#include "file_manager.h"
#include "import_manager.h"

namespace gnarl {

class Scope;

class Workspace final {
public:
    class Context final {
    public:
        Context(Workspace* workspace);
        ~Context();
    };

    struct Options {
        std::string source_dir;
    };

    Workspace(Options options);
    static Workspace* current();

    Scope* globals() const { return m_globals; }
    const Path& source_dir() const { return m_source_dir; }
    FileManager* file_manager() { return &m_file_manager; }
    ImportManager* import_manager() { return &m_import_manager; }

private:
    Path m_source_dir;

    // contians all global variables
    Scope* m_globals;

    FileManager m_file_manager;
    ImportManager m_import_manager;
    // TargetManager m_target_manager;
};

}


#endif // GNARL_WORKSPACE_H_

