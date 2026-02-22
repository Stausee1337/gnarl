#ifndef GNARL_INPUT_FILE_H_
#define GNARL_INPUT_FILE_H_

#include <string>

namespace gnarl {

class InputFile {
public:
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

