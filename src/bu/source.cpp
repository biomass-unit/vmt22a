#include "bu/utilities.hpp"
#include "source.hpp"


bu::Source::Source(std::string&& name)
    : filename { std::move(name) }
{
    if (std::ifstream file { filename }) {
        contents.assign(std::istreambuf_iterator<char> { file }, {});
    }
    else {
        throw Exception { std::format("The file '{}' could not be opened", filename) };
    }
}

bu::Source::Source(Mock_tag const mock_tag, std::string&& contents)
    : filename { std::format("[{}]", mock_tag.filename) }
    , contents { std::move(contents) } {}


auto bu::Source::name() const noexcept -> std::string_view {
    return filename;
}

auto bu::Source::string() const noexcept -> std::string_view {
    return contents;
}