#include "bu/utilities.hpp"
#include "color.hpp"


namespace {

    auto do_color_formatting() noexcept -> bool& {
        static bool state = true; // Avoids SIOF
        return state;
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
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#define INVALID_HANDLE_VALUE               ((HANDLE)(LONG_PTR)-1)


namespace {

    bool has_been_enabled = false;

    auto enable_virtual_terminal_processing() -> void {
        if (!has_been_enabled) {
            has_been_enabled = true;

            // If the win32api calls fail, silently fall back and disable color formatting

            HANDLE const console = GetStdHandle(STD_OUTPUT_HANDLE);
            if (console == INVALID_HANDLE_VALUE) {
                do_color_formatting() = false;
                return;
            }

            DWORD mode;
            if (!GetConsoleMode(console, &mode)) {
                do_color_formatting() = false;
                return;
            }

            if (!(mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
                if (!SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
                    do_color_formatting() = false;
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
    do_color_formatting() = state;
}


DEFINE_FORMATTER_FOR(bu::Color) {
    if (!do_color_formatting()) {
        return context.out();
    }

    enable_virtual_terminal_processing();

    if (!do_color_formatting()) { // must be checked before and after
        return context.out();
    }

    // ANSI color codes
    static constexpr auto color_map = std::to_array<std::string_view>({
        "31", "32", "33", "34", "35", "36",
        "91", "92", "93", "94", "95", "96",
        "30", "0", "90"
    });
    static_assert(color_map.size() == static_cast<bu::Usize>(bu::Color::_color_count));

    return std::format_to(context.out(), "\033[{}m", color_map[static_cast<bu::Usize>(value)]);
}