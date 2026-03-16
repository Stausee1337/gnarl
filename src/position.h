
#ifndef GNARL_SOURCE_H_
#define GNARL_SOURCE_H_

#include <stdint.h>

namespace gnarl {

class InputFile;

struct Position final {

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

struct Span final {
    Span() = default;

    Span(const Position& start, const Position& end)
        : m_file(start.file() ? start.file() : end.file()),
        m_start_lineno(start.lineno()),
        m_start_column(start.column()),
        m_end_lineno(end.lineno()),
        m_end_column(end.column())
    {}

    const InputFile* file() const {
        return m_file;
    }

    Position start() const {
        return Position(m_file, m_start_lineno, m_start_column);
    }

    Position end() const {
        return Position(m_file, m_end_lineno, m_end_column);
    }

private:
    const InputFile* m_file = nullptr;
    uint32_t m_start_lineno;
    uint32_t m_start_column;

    uint32_t m_end_lineno;
    uint32_t m_end_column;

};

}

#endif // GNARL_SOURCE_H_

