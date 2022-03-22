// Only included by ast/ast.hpp

#if 0


namespace ast {

    struct [[nodiscard]] Namespace {
        template <class Definition>
        using Table = ::ast::Table<Definition, std::list>; // std::list is used to avoid pointer invalidation

        Table<definition::Function>           function_definitions;
        Table<definition::Function_template>  function_template_definitions;
        Table<definition::Struct>             struct_definitions;
        Table<definition::Struct_template>    struct_template_definitions;
        Table<definition::Data>               data_definitions;
        Table<definition::Data_template>      data_template_definitions;
        Table<definition::Alias>              alias_definitions;
        Table<definition::Alias_template>     alias_template_definitions;
        Table<definition::Typeclass>          class_definitions;
        Table<definition::Typeclass_template> class_template_definitions;
        Table<Namespace>                      children;
        Namespace*                            parent;
        Namespace*                            global;
        lexer::Identifier                     name;

        using Definition_pointer_variant = std::variant<
            definition::Function*,
            definition::Function_template*,
            definition::Struct*,
            definition::Struct_template*,
            definition::Data*,
            definition::Data_template*,
            definition::Alias*,
            definition::Alias_template*,
            definition::Typeclass*,
            definition::Typeclass_template*,
            Namespace*
        >;

        std::vector<Definition_pointer_variant> definitions_in_order;


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
            definition::Typeclass*,
            definition::Typeclass_template*
        >;

        using Lower_variant = std::variant<
            std::monostate,
            definition::Function*,
            definition::Function_template*
        >;


        auto find_upper(Qualified_name&) -> Upper_variant;
        auto find_lower(Qualified_name&) -> Lower_variant;


        auto handle_recursively(std::invocable<Namespace&> auto&& f) -> void {
            f(*this);
            for (auto& [_, child] : children.container()) {
                child.handle_recursively(f);
            }
        }
    };

}

#endif