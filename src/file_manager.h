
#ifndef GNARL_FILE_MANAGER_H_
#define GNARL_FILE_MANAGER_H_

#include <string>
#include <vector>
#include "input_file.h"

namespace gnarl {

class Error;

class FileManager final {
public:
    FileManager(std::string source_dir)
        : m_source_dir(source_dir)
    {}

    const InputFile* load_file(std::string_view path, Error* error);

private:
    std::string m_source_dir;
    std::vector<InputFile> m_files;
};

}

#endif // GNARL_FILE_MANAGER_H_
