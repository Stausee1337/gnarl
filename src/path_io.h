

#ifndef GNARL_PATH_IO_H_
#define GNARL_PATH_IO_H_

#include <string>

namespace gnarl {

std::string normalize(std::string_view path, const std::string& source_dir = std::string());

}

#endif // GNARL_PATH_IO_H_

