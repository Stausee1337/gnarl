
#ifndef GNARL_WORKSPACE_H_
#define GNARL_WORKSPACE_H_

namespace gnarl {

class Scope;

class Workspace final {
public:
    struct Options {

    };

    Workspace(Options options);

    const Scope* globals() const { return m_globals; }
private:

    // contians all global variables
    const Scope* m_globals;

    // ImportManager m_import_manager;
    // TargetManager m_target_manager;
};

}


#endif // GNARL_WORKSPACE_H_

