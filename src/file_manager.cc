
#include <string.h>
#include "error.h"
#include "file_manager.h"

namespace gnarl {

std::string read_entire_file(const char* filename, Error* error) {
    std::string result;

    FILE* file = fopen(filename, "r");
    long int fsize;

    if (file == NULL) goto failure;
    if (fseek(file, 0L, SEEK_END) != 0) goto failure;

    fsize = ftell(file);
    if (fsize == -1) goto failure;

    result.resize(fsize);
    if (fseek(file, 0L, SEEK_SET) != 0) goto failure;
    if (fread(result.data(), 1, fsize, file) == 0 && fsize > 0) goto failure;

    if (fclose(file) != 0) goto failure;

    return result;

failure:
    *error = Error("Could not read file: " + std::string(filename) + ": " + std::string(strerror(errno)));
    return std::string();
}

const InputFile* FileManager::load_file(Path path, Error* error) {
    std::string data = read_entire_file(path.c_str(), error);
    if (error->has_error())
        return nullptr;
    return &m_files.emplace_back(path, data);
}

}

