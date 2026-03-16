
#ifndef GNARL_FILE_SCOPE_H_
#define GNARL_FILE_SCOPE_H_

#include "scope.h"

namespace gnarl {

class Workspace;

class FileScope final : public Scope {
public:
    FileScope(Workspace* workspace, const InputFile* input_file);

    FileScope(FileScope&&) = delete;
    FileScope& operator=(FileScope&&) = delete;

    Workspace* workspace() const { return m_workspace; }
    const InputFile* input_file() const { return m_input_file; }
private:

    Workspace* m_workspace;
    const InputFile* m_input_file;
};

}

#endif // GNARL_FILE_SCOPE_H_

