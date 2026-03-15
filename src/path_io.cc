
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include "assertions.h"
#include "path_io.h"

namespace gnarl {

class PathParser {
public:
    struct Component {
        enum Kind {
            NORMAL,
            ROOT_DIR,
            SOURCE_DIR,
            PARENT_DIR,
        };

        Component() = default;

        Component(Kind kind, std::string_view data)
            : kind(kind),
            data(data)
        {}

        bool operator==(Component& other) {
            return kind == other.kind && data == other.data;
        }
    
        Kind kind;
        std::string_view data;
    };

    using Iterator = std::string_view::const_iterator;
    PathParser(const std::string_view path)
        : m_path(path),
        m_current(path.begin())
    {}

    // TODO: a lot of windows specific path handling
    //      - prefixes (`C:\`, `\\?\`)
    //      - multiple seperators (`/`, `\`)

    bool advance(Component& component) {
repeat:
        if (m_current >= m_path.end())
            return false;
        else if (m_current == m_path.begin()) {
            if (*m_current == '/') {
                m_current++;
                if (m_current < m_path.end() && *m_current == '/')
                    component = Component(Component::SOURCE_DIR, "//");
                else
                    component = Component(Component::ROOT_DIR, "/");
                return true;
            }
        }
        if (parse_next_component(component)) 
            return true;
        goto repeat;
    }

private:
    bool parse_next_component(Component& component) {
        Iterator start = m_current;
        m_current = std::find(m_current, m_path.end(), '/');

        std::string_view data(&*start, m_current - start);
        if (m_current != m_path.end())
            m_current++;

        if (data == "") return false;
        else if (data == ".") return false;
        else if (data == "..") {
            component = Component(Component::PARENT_DIR, data);
            return true;
        }
        component = Component(Component::NORMAL, data);
        return true;
    }

    const std::string_view m_path;
    Iterator m_current;
};


std::string normalize(std::string_view path, const std::string& source_dir) {
    if (path.size() == 0) return "";

    PathParser path_parser(path);
    PathParser source_dir_parser(source_dir);

    std::vector<PathParser::Component> components;

    PathParser::Component component;
    if (!path_parser.advance(component))
        return "";

    // TODO: maybe figure out a way to write without goto?
    PathParser* parser = &path_parser;
    if (component.kind == PathParser::Component::SOURCE_DIR)
        parser = &source_dir_parser;
    else
        goto do_parse;

    while (parser->advance(component)) {
do_parse:
        if (component.kind != PathParser::Component::PARENT_DIR) {
            components.push_back(component);
            continue;
        }
        PathParser::Component::Kind* prev_kind = &(components.end() - 1)->kind;
        if (components.size() > 1 && *prev_kind != PathParser::Component::PARENT_DIR)
            components.pop_back();
        else if (components.size() == 1 && 
                *prev_kind != PathParser::Component::ROOT_DIR &&
                *prev_kind != PathParser::Component::PARENT_DIR)
            components.pop_back();
        else if (*prev_kind != PathParser::Component::ROOT_DIR)
            components.push_back(component);
    }

    if (path_parser.advance(component)) {
        parser = &path_parser;
        goto do_parse;
    }

    std::string normalized;
    bool is_absolute = components[0].kind == PathParser::Component::ROOT_DIR;

    for (auto iterator = components.begin(); iterator != components.end(); ++iterator) {
        if (iterator != components.begin()) {
            if (!is_absolute)
                normalized.push_back('/');
            else
                is_absolute = false;
        }
        normalized.insert(normalized.end(), iterator->data.begin(), iterator->data.end());
    }
    return normalized;
}

bool is_absolute(std::string_view path) {
    // TODO: windows
    return path.size() >= 1 && path[0] == '/';
}

}

