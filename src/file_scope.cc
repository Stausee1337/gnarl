
#include "workspace.h"
#include "file_scope.h"

namespace gnarl {

FileScope::FileScope(Workspace* workspace) 
    : Scope(workspace->globals()),
    m_workspace(workspace) {
    m_file = this;
}

}

