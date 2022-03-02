// Only included by ast/ast.hpp


namespace ast {

    struct [[nodiscard]] Namespace {
        Table<definition::Function>          function_definitions;
        Table<definition::Function_template> function_template_definitions;
        Table<definition::Struct>            struct_definitions;
        Table<definition::Struct_template>   struct_template_definitions;
        Table<definition::Data>              data_definitions;
        Table<definition::Data_template>     data_template_definitions;
        Table<definition::Alias>             alias_definitions;
        Table<definition::Alias_template>    alias_template_definitions;
        Table<definition::Typeclass>         class_definitions;
        Table<Namespace>                     children;
        Namespace*                           parent;
        Namespace*                           global;
        lexer::Identifier                    name;

        explicit Namespace(lexer::Identifier) noexcept;

        auto make_child(lexer::Identifier) noexcept -> Namespace*;


        using Upper_variant = std::variant<
            std::monostate,

            definition::Struct*,
            definition::Struct_template*,

            definition::Data*,
            definition::Data_template*,

            definition::Alias*,
            definition::Alias_template*,

            definition::Typeclass*
        >;

        using Lower_variant = std::variant<
            std::monostate,

            definition::Function*,
            definition::Function_template*
        >;

        auto find_upper(Qualified_name&) -> Upper_variant;
        auto find_lower(Qualified_name&) -> Lower_variant;
    };

}