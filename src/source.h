
#ifndef GNARL_SOURCE_H_
#define GNARL_SOURCE_H_

#include <stdint.h>
#include <string>

namespace gnarl {

class SourceFile {
public:

    bool is_empty() const {
        return m_source.empty();
    }

    const std::string& source() const {
        return m_source;
    }

private:

    std::string m_source;

};

struct Position {
    Position(const SourceFile* file, uint32_t lineno, uint32_t column)
        : m_file(file),
        m_lineno(lineno),
        m_column(column)
    {}

    const SourceFile* file() const {
        return m_file;
    }

    uint32_t lineno() const {
        return m_lineno;
    }

    uint32_t column() const {
        return m_column;
    }

private:
    const SourceFile* m_file = nullptr;
    uint32_t m_lineno;
    uint32_t m_column;
};

}

#endif // GNARL_SOURCE_H_

