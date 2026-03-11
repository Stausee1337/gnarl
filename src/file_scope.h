
#ifndef GNARL_FILE_SCOPE_H_
#define GNARL_FILE_SCOPE_H_

#include "scope.h"

namespace gnarl {

class Workspace;

class FileScope final : public Scope {
public:
    FileScope(Workspace*);

    FileScope(FileScope&&) = delete;
    FileScope& operator=(FileScope&&) = delete;

    Workspace* workspace() const { return m_workspace; }
private:

    Workspace* m_workspace;
};

}

#endif // GNARL_FILE_SCOPE_H_
