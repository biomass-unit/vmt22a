#include "bu/utilities.hpp"
#include "configuration.hpp"


namespace {

    constexpr auto trim(std::string_view string) -> std::string_view {
        if (string.find_first_not_of(' ') != std::string_view::npos) {
            string.remove_prefix(string.find_first_not_of(' '));
        }
        if (!string.empty()) {
            string.remove_suffix(string.size() - string.find_last_not_of(' ') - 1);
        }
        return string;
    }

    static_assert(trim("       test   ") == "test");
    static_assert(trim("      test"    ) == "test");
    static_assert(trim("test     "     ) == "test");
    static_assert(trim("test"          ) == "test");
    static_assert(trim("     "         ) == ""    );
    static_assert(trim(""              ) == ""    );


    constexpr auto allowed_keys = std::to_array<std::string_view>({
        "src-dir",
        "authors",
        "created"
    });

}


auto language::read_configuration() -> Configuration {
    auto configuration_path = std::filesystem::current_path() / "vmt22a_config";

    Configuration configuration;
    configuration.container().reserve(10);

    if (std::ifstream file { configuration_path }) {
        std::string line;
        line.reserve(50);

        bu::Usize line_number = 0;

        while (std::getline(file, line)) {
            ++line_number;

            if (trim(line).empty()) {
                continue;
            }

            auto components = line
                            | std::views::split(':')
                            | std::views::transform(trim)
                            | bu::ranges::to<std::vector>;

            switch (components.size()) {
            case 1:
                throw bu::exception("vmt22a_config: Expected a ':' after the key '{}'", components.front());
            case 2:
                break;
            default:
                throw bu::exception("vmt22a_config: Only one ':' is allowed per line: '{}'", trim(line));
            }

            auto const key = components.front();
            auto const value = components.back();

            if (key.empty()) {
                throw bu::exception(
                    "vmt22a_config: empty key on the {} line",
                    bu::fmt::integer_with_ordinal_indicator(line_number)
                );
            }
            if (value.empty()) {
                throw bu::exception(
                    "vmt22a_config: empty value on the {} line",
                    bu::fmt::integer_with_ordinal_indicator(line_number)
                );
            }
            if (!bu::ranges::contains(allowed_keys, key)) {
                throw bu::exception(
                    "vmt22a_config: '{}' is not a recognized configuration key",
                    key
                );
            }

            configuration.add(std::string(key), std::string(value));
        }

        return configuration;
    }
    else {
        bu::abort("no config"); // provide default settings
    }
}