#include "bu/utilities.hpp"
#include "color.hpp"


namespace {

    constinit bool color_formatting_state = false;

    constexpr auto color_string(bu::Color const color) noexcept -> std::string_view {
        constexpr auto color_map = std::to_array<std::string_view>({
            "\033[31m",
            "\033[32m",
            "\033[33m",
            "\033[34m",
            "\033[35m",
            "\033[36m",
            "\033[91m",
            "\033[92m",
            "\033[93m",
            "\033[94m",
            "\033[95m",
            "\033[96m",
            "\033[30m",
            "\033[0m" ,
            "\033[90m",
        });
        static_assert(color_map.size() == static_cast<bu::Usize>(bu::Color::_color_count));

        return color_map[static_cast<bu::Usize>(color)];
    }

}


#ifdef _WIN32

// Copied the necessary declarations from Windows.h, this allows not #including it.

extern "C" {
    typedef unsigned long DWORD;
    typedef int           BOOL;
    typedef void*         HANDLE;
    typedef DWORD*        LPDWORD;
    typedef long long     LONG_PTR;

    HANDLE __declspec(dllimport) GetStdHandle(DWORD);
    BOOL   __declspec(dllimport) SetConsoleMode(HANDLE, DWORD);
    BOOL   __declspec(dllimport) GetConsoleMode(HANDLE, LPDWORD);
}

#define STD_OUTPUT_HANDLE                  ((DWORD)-11)
#define INVALID_HANDLE_VALUE               ((HANDLE)(LONG_PTR)-1)
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004


namespace {

    auto enable_virtual_terminal_processing() -> void {
        static bool has_been_enabled = false;

        if (!has_been_enabled) {
            has_been_enabled = true;

            // If the win32api calls fail, silently fall back and disable color formatting

            HANDLE const console = GetStdHandle(STD_OUTPUT_HANDLE);
            if (console == INVALID_HANDLE_VALUE) {
                color_formatting_state = false;
                return;
            }

            DWORD mode;
            if (!GetConsoleMode(console, &mode)) {
                color_formatting_state = false;
                return;
            }

            if (!(mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
                if (!SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
                    color_formatting_state = false;
                    return;
                }
            }
        }
    }

}

#else
#define enable_virtual_terminal_processing()
#endif


auto bu::enable_color_formatting() noexcept -> void {
    enable_virtual_terminal_processing();
    color_formatting_state = true;
}

auto bu::disable_color_formatting() noexcept -> void {
    color_formatting_state = false;
}


auto operator<<(std::ostream& os, bu::Color const color) -> std::ostream& {
    return color_formatting_state
        ? os << color_string(color)
        : os;
}

DEFINE_FORMATTER_FOR(bu::Color) {
    return color_formatting_state
        ? std::format_to(context.out(), color_string(value))
        : context.out();
}