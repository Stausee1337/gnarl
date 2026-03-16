#ifndef GNARL_INPUT_FILE_H_
#define GNARL_INPUT_FILE_H_

#include <string>
#include <vector>
#include "path_io.h"

namespace gnarl {

class InputFile final {
public:
    InputFile(Path path, std::string source);

    InputFile(const InputFile&) = delete;
    const InputFile& operator=(const InputFile&) = delete;

    InputFile(InputFile&&) = default;
    InputFile& operator=(InputFile&&) = default;

    bool is_empty() const {
        return m_source.empty();
    }

    const std::string& source() const {
        return m_source;
    }

    const Path& path() const {
        return m_path;
    }

    std::string_view get_line(size_t lineno) const;

private:
    void analyze_lines();

    Path m_path;
    std::string m_source;
    std::vector<size_t> m_lines; 
};

}

#endif // GNARL_INPUT_FILE_H_

