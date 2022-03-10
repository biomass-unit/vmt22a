#include "bu/utilities.hpp"
#include "cli.hpp"


namespace {



}


auto cli::parse_command_line(int argc, char const** argv, Options_description const& /*description*/)
    -> Options
{
    std::vector<std::string_view> const command_line(argv + 1, argv + argc);

    Options options { .program_name_as_invoked = *argv };



    return options;
}


DEFINE_FORMATTER_FOR(cli::Options_description) {
    std::vector<bu::Pair<std::string, std::optional<std::string>>> lines;
    bu::Usize max_length = 0;

    for (auto& [long_name, short_name, argument, description] : value.parameters) {
        lines.emplace_back(
            std::format(
                "--{}{}{}",
                long_name,
                std::format(short_name ? ", -{}" : "", short_name),
                std::holds_alternative<std::monostate>(argument) ? "" : " [arg]"
            ),
            description
        );

        max_length = std::max(max_length, lines.back().first.size());
    }

    for (auto& [names, description] : lines) {
        std::format_to(
            context.out(),
            "\t{:{}}{}\n",
            names,
            max_length,
            std::format(description ? " : {}" : "", description)
        );
    }

    return context.out();
}