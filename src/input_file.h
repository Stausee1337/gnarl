#ifndef GNARL_INPUT_FILE_H_
#define GNARL_INPUT_FILE_H_

#include <string>

namespace gnarl {

class InputFile final {
public:
    InputFile(std::string source)
        : m_source(source)
    {}

    InputFile(const InputFile&) = delete;
    const InputFile& operator=(const InputFile&) = delete;

    bool is_empty() const {
        return m_source.empty();
    }

    const std::string& source() const {
        return m_source;
    }

private:
    std::string m_source;
};

}

#endif // GNARL_INPUT_FILE_H_

