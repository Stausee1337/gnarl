#ifndef GNARL_INPUT_FILE_H_
#define GNARL_INPUT_FILE_H_

#include <string>
#include <vector>

namespace gnarl {

class InputFile final {
public:
    InputFile(std::string path, std::string source);

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

    const std::string& path() const {
        return m_path;
    }

    std::string_view get_line(size_t lineno) const;

private:
    void analyze_lines();

    std::string m_path;
    std::string m_source;
    std::vector<size_t> m_lines; 
};

}

#endif // GNARL_INPUT_FILE_H_

