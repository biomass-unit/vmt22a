// Only included by ast/ast.hpp


namespace ast {

    class [[nodiscard]] Namespace {
        struct Namespace_data {
            std::vector<definition::Function>  function_definitions;
            std::vector<definition::Data>      data_definitions;
            std::vector<definition::Struct>    struct_definitions;
            std::vector<definition::Typeclass> class_definitions;
            std::vector<Namespace>             children;
            Namespace_data*                    parent;
            lexer::Identifier                  name;
        };
        bu::Wrapper<Namespace_data> data;
    public:
        explicit Namespace(lexer::Identifier) noexcept;

        auto make_child(lexer::Identifier) noexcept -> Namespace;

        auto operator->()       noexcept -> Namespace_data      *;
        auto operator->() const noexcept -> Namespace_data const*;
    };

}