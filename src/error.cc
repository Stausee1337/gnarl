
#include <memory>

#include "assertions.h"
#include "error.h"

namespace gnarl {

Error::Error(const Position& position, std::string message, std::string help)
    : data(std::make_unique<Data>(position, message, help))
{}

Error::Error(const Error& other) {
    if (other.has_error())
        data = std::make_unique<Data>(*other.data);
}

void Error::append_suberror(const Error& suberror) {
    DCHECK(has_error());
    data->suberrors.push_back(suberror);
}

}

