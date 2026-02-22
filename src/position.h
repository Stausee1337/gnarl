
#ifndef GNARL_SOURCE_H_
#define GNARL_SOURCE_H_

#include <stdint.h>

#include "input_file.h"

namespace gnarl {
class InputFile;


struct Position {

    Position() = default;

    Position(const InputFile* file, uint32_t lineno, uint32_t column)
        : m_file(file),
        m_lineno(lineno),
        m_column(column)
    {}

    const InputFile* file() const {
        return m_file;
    }

    uint32_t lineno() const {
        return m_lineno;
    }

    uint32_t column() const {
        return m_column;
    }

private:
    const InputFile* m_file = nullptr;
    uint32_t m_lineno;
    uint32_t m_column;
};

}

#endif // GNARL_SOURCE_H_

