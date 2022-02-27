// Only included by ast/ast.hpp


namespace ast {

    struct [[nodiscard]] Namespace {
        template <class T>
        using Table = bu::Flatmap<lexer::Identifier, T, bu::Flatmap_strategy::store_keys>;

        Table<definition::Function>  function_definitions;
        Table<definition::Data>      data_definitions;
        Table<definition::Struct>    struct_definitions;
        Table<definition::Alias>     alias_definitions;
        Table<definition::Typeclass> class_definitions;
        Table<Namespace>             children;
        Namespace*                   parent;
        Namespace*                   global;
        lexer::Identifier            name;

        explicit Namespace(lexer::Identifier) noexcept;

        auto make_child(lexer::Identifier) noexcept -> Namespace*;

        auto find_type_or_typeclass(Qualified_name&)
            -> std::variant<std::monostate, definition::Struct*, definition::Data*, definition::Typeclass*>;

        auto find_function_or_constructor(Qualified_name&)
            -> std::variant<std::monostate, definition::Function*/*, ast::definition::Data::Constructor**/>;
    };

}