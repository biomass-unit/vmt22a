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


    constexpr auto remove_comments(std::string_view string) -> std::string_view {
        if (bu::Usize const offset = string.find("//"); offset != std::string_view::npos) {
            string.remove_suffix(string.size() - offset);
        }
        return string;
    }

    static_assert(remove_comments("test//test") == "test");
    static_assert(remove_comments("test/test") == "test/test");
    static_assert(remove_comments("test") == "test");
    static_assert(remove_comments("// test") == "");
    static_assert(remove_comments("//") == "");
    static_assert(remove_comments("") == "");


    constexpr auto allowed_keys = std::to_array<std::string_view>({
        "language version",
        "source directory",
        "stack capacity",
        "name",
        "version",
        "authors",
        "created"
    });

}


auto language::Configuration::string() const -> std::string {
    std::string string;
    string.reserve(256);
    auto out = std::back_inserter(string);

    for (auto const& [key, value] : container()) {
        std::format_to(
            out,
            "{}: {}\n",
            key,
            value.transform(bu::compose(bu::copy, &Configuration_key::string)).value_or(""s)
        );
    }

    return string;
}


auto language::Configuration::operator[](std::string_view const name) -> Configuration_key& {
    bu::always_assert(bu::ranges::contains(allowed_keys, name));

    if (auto* const key = find(name); key && *key) {
        return **key;
    }
    else {
        bu::abort();
    }
}


auto language::default_configuration() -> Configuration {
    return Configuration { {
        { "language version", std::to_string(version) },
        { "source directory", "src"s },
        { "stack capacity", "1048576 // 2^20"s },
        { "name", std::nullopt },
        { "version", std::nullopt },
        { "authors", std::nullopt },
        { "created", "{:%d-%m-%Y}"_format(bu::local_time()) }
    } };
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
                            | std::views::transform(bu::compose(&trim, &remove_comments))
                            | bu::ranges::to<std::vector>();

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

            if (!bu::ranges::contains(allowed_keys, key)) {
                throw bu::exception(
                    "vmt22a_config: '{}' is not a recognized configuration key",
                    key
                );
            }

            if (configuration.find(key)) {
                throw bu::exception(
                    "vmt22a_config: '{}' key redefinition on the {} line",
                    key,
                    bu::fmt::integer_with_ordinal_indicator(line_number)
                );
            }

            configuration.add(
                std::string(key),
                value.empty()
                    ? std::optional(std::string(value))
                    : std::nullopt
            );
        }

        return configuration;
    }
    else {
        return default_configuration();
    }
}