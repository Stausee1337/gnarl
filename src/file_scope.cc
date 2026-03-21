
#include "workspace.h"
#include "file_scope.h"

namespace gnarl {

FileScope::FileScope(Workspace* workspace, const InputFile* input_file) 
    : Scope((const Scope*)workspace->globals()),
    m_workspace(workspace),
    m_input_file(input_file) {
    m_file = this;
}

}

