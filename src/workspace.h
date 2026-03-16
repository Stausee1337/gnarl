
#ifndef GNARL_WORKSPACE_H_
#define GNARL_WORKSPACE_H_

#include <string>
#include "file_manager.h"

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

    const Scope* globals() const { return m_globals; }
    FileManager* file_manager() { return &m_file_manager; }
private:

    // contians all global variables
    const Scope* m_globals;

    FileManager m_file_manager;
    // ImportManager m_import_manager;
    // TargetManager m_target_manager;
};

}


#endif // GNARL_WORKSPACE_H_

