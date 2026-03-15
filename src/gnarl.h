
#ifndef GNARL_GNARL_H_
#define GNARL_GNARL_H_

#include "workspace.h"

namespace gnarl {

enum ExitCode : int {
    SUCCESS,
    FAILIURE,
};

void run_gnarl(Workspace* workspace, ExitCode* exit_code);
    
}

#endif // GNARL_GNARL_H_

