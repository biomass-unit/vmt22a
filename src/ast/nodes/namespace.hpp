// Only included by ast/ast.hpp


namespace ast {

    struct [[nodiscard]] Namespace {
        std::vector<definition::Function>  function_definitions;
        std::vector<definition::Data>      data_definitions;
        std::vector<definition::Struct>    struct_definitions;
        std::vector<definition::Typeclass> class_definitions;
        std::vector<Namespace>             children;
        Namespace*                         parent;
        lexer::Identifier                  name;

        explicit Namespace(lexer::Identifier) noexcept;

        auto make_child(lexer::Identifier) noexcept -> Namespace*;
    };

}