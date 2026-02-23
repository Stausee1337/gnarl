
#include <memory>

#include "error.h"
#include "nodes.h"

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

}

