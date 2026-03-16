
#ifndef GNARL_FILE_MANAGER_H_
#define GNARL_FILE_MANAGER_H_

#include <string>
#include <vector>
#include "path_io.h"
#include "input_file.h"

namespace gnarl {

class Error;

class FileManager final {
public:
    FileManager() = default;

    const InputFile* load_file(PathView path, Error* error);

private:
    std::vector<InputFile> m_files;
};

}

#endif // GNARL_FILE_MANAGER_H_
