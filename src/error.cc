
#include <memory>

#include "error.h"
#include "nodes.h"
#include "format.h"

namespace gnarl {

Error::Error(const Position& position, std::string message, std::string help)
    : data(std::make_unique<Data>(position, message, help))
{}

Error::Error(const Span& span, std::string message, std::string help)
    : data(std::make_unique<Data>(span.start(), message, help)) {
    append_span(span);
}

Error::Error(const BaseNode* node, std::string message, std::string help)
    : data(std::make_unique<Data>(Position(), message, help)) {
    if (node) {
        const Span& span = node->get_span();
        data->position = span.start();
        append_span(span);
    }
}

Error::Error(const Error& other) {
    if (other.has_error())
        data = std::make_unique<Data>(*other.data);
}

void Error::print_to_stdout() const {
    DCHECK(has_error());

    const InputFile* file = position().file();

    // TODO: format differently if file is null (aka there's no position)
    if (file == nullptr) {
        print("ERROR: {}\n", message());
        DCHECK(file != nullptr);
    }

    print("ERROR at {}:{}:{}: {}\n", 
            file->path(),
            position().lineno(),
            position().column(),
            message()
    );

    std::string_view source = file->get_line(position().lineno());
    print("{}\n", source);

    size_t offset = position().column() - 1;
    if (data->spans.size()) {
        size_t length = data->spans.size() && data->spans[0].end().lineno() == position().lineno()
            ? data->spans[0].end().column() - position().column() - 1
            : source.length() - position().column();

        print("{:>$}{:->$}\n", "^", offset, "", length);
    } else {
        print("{:>$}\n", "^", offset);
    }

    if (help().size())
        print("{}\n", help());

}

}

