#include "assertions.h"
#include "input_file.h"

namespace gnarl {

InputFile::InputFile(Path path, std::string source)
    : m_path(path),
    m_source(source) {
    analyze_lines();
}

std::string_view InputFile::get_line(size_t lineno) const {
    DCHECK(lineno > 0 && lineno <= m_lines.size());
    size_t offset = m_lines[lineno - 1];
    size_t length;
    if (lineno < m_lines.size())
        length = m_lines[lineno] - offset;
    else
        length = m_source.length() - offset;

    DCHECK((offset + length) <= m_source.size());
    std::string_view view(m_source.data() + offset, length);

    while (view.length()) {
        char c = *(view.end() - 1);
        if (c == '\n' || c == '\r')
            view.remove_suffix(1);
        else
            break;
    }

    return view;
}

void InputFile::analyze_lines() {
    DCHECK(m_lines.size() == 0);
    m_lines.push_back(0);

    const std::string::const_iterator
        begin = m_source.begin(),
        end = m_source.end();
    for (std::string::const_iterator iterator = begin; iterator != end; ++iterator) {
        char c = *iterator;
        size_t offset = iterator - begin;
        if (c == '\n' && iterator + 1 < end)
            m_lines.push_back(offset + 1);
        else if (c == '\r' && iterator + 2 < end && *(iterator + 1) == '\n')
            m_lines.push_back(offset + 2);
        else if (c == '\r')
            m_lines.push_back(offset + 1);
    }
}

}
