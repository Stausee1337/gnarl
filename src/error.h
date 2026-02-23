
#ifndef GNARL_ERROR_H_
#define GNARL_ERROR_H_

#include <memory>
#include <string>
#include <vector>

#include "assertions.h"
#include "position.h"

namespace gnarl {

class BaseNode;

class Error final {
public:

    Error() = default;

    Error(std::string message,
          std::string help = std::string())
        : Error(Position(), message, help)
    {}

    Error(const Position& position,
          std::string message,
          std::string help = std::string());

    Error(const Span& span,
          std::string message,
          std::string help = std::string());

    Error(const BaseNode*,
          std::string message,
          std::string help = std::string());

    Error(const Error&);
    Error& operator=(const Error&);

    Error(Error&&) = default;
    Error& operator=(Error&&) = default;

    bool has_error() const { return data != nullptr; }

    const Position& position() const { return data->position; }
    const std::string& message() const { return data->message; }
    const std::string& help() const { return data->help; }

    void append_span(const Span& span) {
        DCHECK(has_error());
        data->spans.push_back(span);
    }

    void append_suberror(const Error& suberror) {
        DCHECK(has_error());
        data->suberrors.push_back(suberror);
    }

    // void print_to_stdout() const;

private:
    struct Data {
        Data(Position position, std::string message, std::string help) 
            : position(position),
            message(message),
            help(help) {}

        Position position;
        std::string message;
        std::string help;

        std::vector<Error> suberrors;
        std::vector<Span> spans;
    };

    std::unique_ptr<Data> data;
};

}

#endif // GNARL_ERROR_H_

