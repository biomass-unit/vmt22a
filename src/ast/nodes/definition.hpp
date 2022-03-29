// Only included by ast/ast.hpp


namespace ast {

    struct Definition;


    struct Template_argument {
        std::variant<Type, Expression, Mutability> value;
        DEFAULTED_EQUALITY(Template_argument);
    };

    struct Class_reference {
        std::optional<std::vector<Template_argument>> template_arguments;
        Qualified_name                                name;
        DEFAULTED_EQUALITY(Class_reference);
    };

    struct Template_parameter {
        struct Type_parameter {
            std::vector<Class_reference> classes;
            lexer::Identifier            name;
            DEFAULTED_EQUALITY(Type_parameter);
        };
        struct Value_parameter {
            lexer::Identifier name;
            bu::Wrapper<Type> type;
            DEFAULTED_EQUALITY(Value_parameter);
        };
        struct Mutability_parameter {
            lexer::Identifier name;
            DEFAULTED_EQUALITY(Mutability_parameter);
        };
        std::variant<
            Type_parameter,
            Value_parameter,
            Mutability_parameter
        > value;
        DEFAULTED_EQUALITY(Template_parameter);
    };

    struct Function_signature {
        std::optional<std::vector<Template_parameter>> template_parameters;
        type::Function                                 type;
        lexer::Identifier                              name;
        DEFAULTED_EQUALITY(Function_signature);
    };

    struct Type_signature {
        std::optional<std::vector<Template_parameter>> template_parameters;
        std::vector<Class_reference>                   classes;
        lexer::Identifier                              name;
        DEFAULTED_EQUALITY(Type_signature);
    };


    namespace definition {

        struct Function {
            struct Parameter {
                bu::Wrapper<Pattern>                   pattern;
                bu::Wrapper<Type>                      type;
                std::optional<bu::Wrapper<Expression>> default_value;
                DEFAULTED_EQUALITY(Parameter);
            };
            Expression                       body;
            std::vector<Parameter>           parameters;
            lexer::Identifier                name;
            std::optional<bu::Wrapper<Type>> return_type;
            DEFAULTED_EQUALITY(Function);
        };

        struct Struct {
            struct Member {
                lexer::Identifier name;
                bu::Wrapper<Type> type;
                bool              is_public = false;
                DEFAULTED_EQUALITY(Member);
            };
            std::vector<Member> members;
            lexer::Identifier   name;
            DEFAULTED_EQUALITY(Struct);
        };

        struct Data {
            struct Constructor {
                lexer::Identifier                name;
                std::optional<bu::Wrapper<Type>> type;
                DEFAULTED_EQUALITY(Constructor);
            };
            std::vector<Constructor> constructors;
            lexer::Identifier        name;
            DEFAULTED_EQUALITY(Data);
        };

        struct Alias {
            lexer::Identifier name;
            bu::Wrapper<Type> type;
            DEFAULTED_EQUALITY(Alias);
        };

        struct Typeclass {
            std::vector<Function_signature> function_signatures;
            std::vector<Type_signature>     type_signatures;
            lexer::Identifier               name;
            DEFAULTED_EQUALITY(Typeclass);
        };

        struct Implementation {
            bu::Wrapper<Type>       type;
            std::vector<Definition> definitions;
            DEFAULTED_EQUALITY(Implementation);
        };

        struct Instantiation {
            Class_reference         typeclass;
            bu::Wrapper<Type>       instance;
            std::vector<Definition> definitions;
            DEFAULTED_EQUALITY(Instantiation);
        };

        struct Namespace {
            std::vector<Definition> definitions;
            lexer::Identifier       name;
            DEFAULTED_EQUALITY(Namespace);
        };


        template <class Definition>
        struct Template_definition {
            Definition                      definition;
            std::vector<Template_parameter> parameters;
            DEFAULTED_EQUALITY(Template_definition);
        };

        using Function_template       = Template_definition<Function      >;
        using Struct_template         = Template_definition<Struct        >;
        using Data_template           = Template_definition<Data          >;
        using Alias_template          = Template_definition<Alias         >;
        using Typeclass_template      = Template_definition<Typeclass     >;
        using Implementation_template = Template_definition<Implementation>;
        using Instantiation_template  = Template_definition<Instantiation >;
        using Namespace_template      = Template_definition<Namespace     >;

    }


    struct Definition : dtl::Source_tracked {
        using Variant = std::variant<
            definition::Function,
            definition::Function_template,
            definition::Data,
            definition::Data_template,
            definition::Struct,
            definition::Struct_template,
            definition::Alias,
            definition::Alias_template,
            definition::Typeclass,
            definition::Typeclass_template,
            definition::Instantiation,
            definition::Instantiation_template,
            definition::Implementation,
            definition::Implementation_template,
            definition::Namespace,
            definition::Namespace_template
        >;

        Variant value;

        DEFINE_NODE_CTOR(Definition);
    };

}