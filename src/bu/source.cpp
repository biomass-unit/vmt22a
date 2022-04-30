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


auto bu::Source_position::increment_with(char const c) noexcept -> void {
    if (c == '\n') {
        ++line;
        column = 1;
    }
    else {
        ++column;
    }
}


auto bu::Source_view::operator+(Source_view const& other) const noexcept -> Source_view {
    assert(string.empty() || other.string.empty() || &string.front() <= &other.string.back());

    return Source_view {
        std::string_view {
            string.data(),
            other.string.data() + other.string.size()
        },
        start_position,
        other.stop_position
    };
}


static_assert(std::is_trivially_copyable_v<bu::Source_view>);

static_assert(bu::Source_position { 4, 5 } < bu::Source_position { 9, 2 });
static_assert(bu::Source_position { 5, 2 } < bu::Source_position { 5, 3 });
static_assert(bu::Source_position { 3, 2 } > bu::Source_position { 2, 3 });


DEFINE_FORMATTER_FOR(bu::Source_position) {
    return std::format_to(context.out(), "{}:{}", value.line, value.column);
}