
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
        Iterator start = m_current;
        if (*m_current == '/') {
            m_current++;
            if (m_current < m_path.end() && *m_current == '/') {
                m_current++;
                component = Component(Component::SOURCE_DIR, std::string_view(&*start, m_current - start));
            } else
                component = Component(Component::ROOT_DIR, std::string_view(&*start, m_current - start));
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

PathView Path::parent() const {
    return PathView(*this).parent();
}

std::string_view Path::file() const {
    return PathView(*this).file();
}

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

bool PathView::is_absolute() const {
    PathParser parser = components();
    PathParser::Component first;
    if (!parser.advance(first)) return false;
    return first.kind == PathParser::Component::ROOT_DIR;
}

bool PathView::is_source_absolute() const {
    PathParser parser = components();
    PathParser::Component first;
    if (!parser.advance(first)) return false;
    return first.kind == PathParser::Component::SOURCE_DIR;
}

PathView PathView::parent() const {
    PathParser parser = components();

    // TODO: use double sided iterator to improve performance
    PathParser::Component components[3];
    size_t idx;
    for (idx = 0; parser.advance(components[idx % 3]); ++idx);

    if (idx < 2) return "";
    PathParser::Component& component = components[(idx - 2) % 3];
    return PathView(m_data, (component.data.data() - m_data) + component.data.length());
}

std::string_view PathView::file() const {
    if (m_length == 0 || m_data[m_length - 1] == '/') return "";

    PathParser parser = components();

    // TODO: use double sided iterator to improve performance
    PathParser::Component components[2];
    size_t idx;
    for (idx = 0; parser.advance(components[idx % 2]); ++idx);

    if (idx < 1) return "";
    return components[(idx - 1) % 2].data;
}

PathParser PathView::components() const {
    return PathParser(this->string());
}

bool PathView::operator==(const PathView& other) const {
    return m_length == other.m_length && strncmp(m_data, other.m_data, m_length) == 0;
}

PathView source_dir() {
    Workspace* workspace = Workspace::current();
    DCHECK(workspace);
    return workspace->source_dir();
}

Path normalize(PathView path, bool keep_source_dir) {
    if (path.size() == 0) return Path();

    PathParser path_parser = path.components();
    PathParser source_dir_parser = source_dir().components();

    std::vector<PathParser::Component> components;

    PathParser::Component component;
    if (!path_parser.advance(component))
        return Path();

    // TODO: maybe figure out a way to write without goto?
    PathParser* parser = &path_parser;
    if (component.kind == PathParser::Component::SOURCE_DIR && !keep_source_dir)
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


Path normalize(PathView path) {
    return normalize(path, /*keep_source_dir=*/ false);
}

bool strip_prefix(PathView path, PathView prefix, Path& result) {
    PathParser path_parser = path.components();
    PathParser prefix_parser = prefix.components();

    PathParser::Component cpath;
    PathParser::Component cprefix;
    while (true) {

        bool epath = path_parser.advance(cpath);
        bool eprefix = prefix_parser.advance(cprefix);

        if (!eprefix) break;
        if (!epath) return false;
        if (cpath != cprefix) return false;
    }

    do {
        result += cpath;
    } while (path_parser.advance(cpath));

    return true;
}

Path resolve_unique(PathView path, PathView currdir) {
    if (path.is_source_absolute()) {
         return normalize(path, /*keep_source_dir=*/ true);
    } else if (path.is_absolute()) {
         Path absolute = normalize(path);
         Path result("//");
         if (strip_prefix(absolute, source_dir(), result))
             return result;
         return absolute;
    }
    
    DCHECK(currdir.is_absolute() || currdir.is_source_absolute());
    Path absolute = normalize(currdir);
    absolute += path;
    return resolve_unique(absolute, "");
}

std::string_view splitext(std::string_view* filename) {
    if (filename->size() == 0 || *filename == "..") return "";

    std::string_view::const_iterator current = filename->end() - 1;
    for (; current != filename->begin(); --current)
        if (*current == '.') break;

    if (current == filename->begin())
        return "";

    std::string_view ext(current+1, (filename->end() - current) - 1);
    *filename = std::string_view(filename->begin(), (current - filename->begin()));
    return ext;
}

}

