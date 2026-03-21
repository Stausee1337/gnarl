
#ifndef GNARL_PATH_IO_H_
#define GNARL_PATH_IO_H_

#include <string.h>
#include <concepts>
#include <string_view>
#include <vector>
#include <string>

namespace gnarl {

class PathView;

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

        Component(Kind kind, std::string_view data) : kind(kind), data(data)
        {}

        bool operator==(const Component& other) const {
            return kind == other.kind && data == other.data;
        }
    
        Kind kind;
        std::string_view data;
    };


    bool advance(Component& component);

private:
    friend class PathView;

    using Iterator = std::string_view::const_iterator;
    PathParser(std::string_view path);

    bool parse_next_component(Component& component);

    const std::string_view m_path;
    Iterator m_current;
};

class Path final {
public:
    Path() = default;
    Path(const std::string& path) : m_data(path)
    {}

    Path(std::string_view path) : m_data(path)
    {}

    explicit Path(const char* path) : m_data(path, strlen(path))
    {}

    explicit Path(PathView view);

    const char* data() const { return m_data.data(); }
    const char* c_str() const { return m_data.c_str(); }
    size_t size() const { return m_data.size(); }
    std::string_view string() const { return m_data; }

    bool is_absolute() const;
    bool is_source_absolute() const;
    PathView parent() const;
    std::string_view file() const;

    PathParser components() const;

    Path& operator+=(PathView view);

private:
    std::string m_data;
};

class PathView final {
public:
    PathView(const Path& path)
        : m_length(path.size()), m_data(path.data())
    {}

    PathView(const std::string& string)
        : m_length(string.length()), m_data(string.data())
    {}

    PathView(std::string_view path)
        : m_length(path.length()), m_data(path.data())
    {}

    PathView(const char* path)
        : m_length(strlen(path)), m_data(path)
    {}

    PathView(const char* path, size_t length)
        : m_length(length), m_data(path)
    {}

    PathView(const PathParser::Component& component);

    size_t size() const { return m_length; }
    const char* data() const { return m_data; }
    std::string_view string() const { return std::string_view(m_data, m_length); }

    bool is_absolute() const;
    bool is_source_absolute() const;
    PathView parent() const;
    std::string_view file() const;

    PathParser components() const;

    bool operator==(const PathView& other) const;

private:
    size_t m_length;
    const char* m_data;
};

template<typename T>
concept PathLike = std::convertible_to<T, PathView>;

Path normalize(PathView path);

Path resolve_unique(PathView path, PathView currdir);

std::string_view splitext(std::string_view* filename);

bool pathexists(Path path);
bool pathexists(PathView path);
}

namespace std {

template<>
struct hash<gnarl::PathView> {
    size_t operator()(const gnarl::PathView& view) const {
        return hash<std::string_view>()(view.string());
    }
};

}

#endif // GNARL_PATH_IO_H_

