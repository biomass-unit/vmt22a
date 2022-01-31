// Only included by ast/ast.hpp


namespace ast {

    namespace definition {

        struct Function {
            struct Parameter {
                Pattern           pattern;
                bu::Wrapper<Type> type;
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

    }


    struct Template_argument {
        std::variant<Type, Expression> value;
        DEFAULTED_EQUALITY(Template_argument);
    };

    struct Class_reference {
        std::optional<std::vector<Template_argument>> arguments;
        lexer::Identifier                             name;
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
        std::variant<Type_parameter, Value_parameter> value;
        DEFAULTED_EQUALITY(Template_parameter);
    };


    namespace definition {

        template <class Definition>
        struct Template_definition {
            Definition                      definition;
            std::vector<Template_parameter> parameters;
            DEFAULTED_EQUALITY(Template_definition);
        };

        using Function_template = Template_definition<Function>;
        using Struct_template   = Template_definition<Struct>;
        using Data_template     = Template_definition<Data>;

    }


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

        struct Typeclass {
            std::vector<Function_signature> function_signatures;
            std::vector<Type_signature>     type_signatures;
            lexer::Identifier               name;
            DEFAULTED_EQUALITY(Typeclass);
        };

    }

}