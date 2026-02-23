
#include <memory>

#include "error.h"

namespace gnarl {

Error::Error(const Position& position, std::string message, std::string help)
    : data(std::make_unique<Data>(position, message, help))
{}

Error::Error(const Span& span, std::string message, std::string help)
    : data(std::make_unique<Data>(span.start(), message, help)) {
    append_span(span);
}

Error::Error(const Error& other) {
    if (other.has_error())
        data = std::make_unique<Data>(*other.data);
}

}

