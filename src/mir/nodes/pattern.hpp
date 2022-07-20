#ifndef VMT22A_MIR_NODES_PATTERN
#define VMT22A_MIR_NODES_PATTERN
#else
#error This isn't supposed to be included by anything other than mir/mir.hpp
#endif


namespace mir {

    namespace pattern {

        struct Wildcard {};

    }


    struct Pattern {
        using Variant = std::variant<
            pattern::Wildcard
        >;

        Variant                        value;
        std::optional<bu::Source_view> source_view;
    };

}