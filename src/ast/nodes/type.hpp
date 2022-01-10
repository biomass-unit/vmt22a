// Only included by ast/ast.hpp


namespace ast {

    namespace type {

        template <class>
        struct Primitive {};

    }


    struct Type {
        using Variant = std::variant<
            type::Primitive<bu::Isize>,
            type::Primitive<bu::Float>,
            type::Primitive<bu::Char>,
            type::Primitive<bool>
        >;
        Variant value;
        DEFAULTED_EQUALITY(Type);
    };

}