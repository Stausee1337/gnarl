#include <filesystem>
#include <iostream>

#include "gnarl.h"

int main() {
    gnarl::Workspace::Options options {
        .source_dir = std::filesystem::current_path(),
    };
    gnarl::Workspace workspace(options);

    gnarl::ExitCode code;
    gnarl::run_gnarl(&workspace, &code);

    return code;
}

