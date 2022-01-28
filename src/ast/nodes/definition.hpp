// Only included by ast/ast.hpp


namespace ast {

    namespace definition {

        struct Function {
            struct Parameter {
                Pattern           pattern;
                bu::Wrapper<Type> type;
                DEFAULTED_EQUALITY(Parameter);
            };
            Expression             body;
            std::vector<Parameter> parameters;
            bu::Wrapper<Type>      return_type;
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


        struct Template_parameter {
            struct Type_parameter {
                lexer::Identifier name;
                std::vector<lexer::Identifier> classes; // todo: add way to reference typeclass
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

        struct Template_argument {
            std::variant<Type, Expression> value;
            DEFAULTED_EQUALITY(Template_argument);
        };


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

}