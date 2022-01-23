#include "bu/utilities.hpp"
#include "color.hpp"


namespace {

    constinit bool color_formatting_state = true;

    constexpr auto color_code(bu::Color const color) noexcept -> std::string_view {
        // ANSI color codes
        constexpr auto color_map = std::to_array<std::string_view>({
            "31", "32", "33", "34", "35", "36",
            "91", "92", "93", "94", "95", "96",
            "30", "0", "90"
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


auto bu::use_color_formatting(bool const state) -> void {
    color_formatting_state = state;
}


DEFINE_FORMATTER_FOR(bu::Color) {
    if (!color_formatting_state) {
        return context.out();
    }

    enable_virtual_terminal_processing();

    if (!color_formatting_state) { // must be checked twice
        return context.out();
    }

    return std::format_to(context.out(), "\033[{}m", color_code(value));
}