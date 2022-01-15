#include "bu/utilities.hpp"
#include "source.hpp"

#include <fstream>


bu::Source::Source(std::string&& name)
    : filename { std::move(name) }
{
    if (std::ifstream file { filename }) {
        contents.assign(std::istreambuf_iterator<char> { file }, {});
    }
    else {
        throw std::runtime_error { std::format("The file '{}' could not be opened", filename) };
    }
}


auto bu::Source::string() const noexcept -> std::string_view {
    return contents;
}