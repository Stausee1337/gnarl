
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include "assertions.h"
#include "workspace.h"
#include "path_io.h"

namespace gnarl {

PathParser::PathParser(std::string_view path)
    : m_path(path),
    m_current(path.begin())
{}

bool PathParser::advance(Component& component) {
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

bool PathParser::parse_next_component(Component& component) {
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

Path::Path(PathView view) 
    : m_data(view.data(), view.size())
{ }

PathParser Path::components() const {
    return PathView(*this).components();
}

Path& Path::operator+=(PathView view) {
    if (m_data.length() > 0 && m_data.back() != '/')
        m_data.push_back('/');
    m_data.insert(m_data.length(), view.data(), view.size());
    return *this;
}

PathView::PathView(const PathParser::Component& component) {
    switch (component.kind) {
        case PathParser::Component::NORMAL:
            m_length = component.data.size();
            m_data = component.data.data();
            break;
        case PathParser::Component::ROOT_DIR:
            m_length = 1;
            m_data = "/";
            break;
        case PathParser::Component::SOURCE_DIR:
            m_length = 2;
            m_data = "//";
            break;
        case PathParser::Component::PARENT_DIR:
            m_length = 2;
            m_data = "..";
            break;
    }
}

PathParser PathView::components() const {
    return PathParser(this->string());
}

PathView source_dir() {
    Workspace* workspace = Workspace::current();
    DCHECK(workspace);
    return workspace->source_dir();
}

Path normalize(PathView path) {
    if (path.size() == 0) return Path();

    PathParser path_parser = path.components();
    PathParser source_dir_parser = source_dir().components();

    std::vector<PathParser::Component> components;

    PathParser::Component component;
    if (!path_parser.advance(component))
        return Path();

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

    Path normalized; 
    for (auto& component : components)
        normalized += component;

    return normalized;
}

}

