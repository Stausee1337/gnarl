
#include "scope.h"
#include "value.h"
#include "workspace.h"

namespace gnarl {

// TODO: maybe factor into a `globals.h`
void setup_gnarl_globals(Scope* globals) {
    globals->set_value("gnarl_version", Value(nullptr, (int64_t)100)); 
}

Workspace::Workspace(Workspace::Options options)
    : m_source_dir(options.source_dir),
    m_import_manager(&m_file_manager) {
    m_globals = new Scope();
    setup_gnarl_globals(m_globals);
}

static Workspace* current_workspace = nullptr;

Workspace* Workspace::current() {
    return current_workspace;
}

Workspace::Context::Context(Workspace* workspace) {
    DCHECK(!current_workspace);
    current_workspace = workspace;
}

Workspace::Context::~Context() {
    DCHECK(current_workspace);
    current_workspace = nullptr;
}

}

