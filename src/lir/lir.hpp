#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "lexer/token.hpp"


namespace lir {

    struct [[nodiscard]] Expression;


    // Offset from the beginning of the code section of the current (possibly composite) module. Used for constants such as function addresses.
    struct Module_offset {
        bu::U64 value = 0;
    };

    // Offset from the current frame pointer. Used for local addressing on the stack.
    struct Frame_offset {
        bu::I64 value = 0;
    };

    // Jump offset from the current instruction pointer. Used for local jumps that arise from things like `if` or `loop` expressions.
    struct Local_jump_offset {
        bu::I16 value = 0;
    };


    namespace expression {

        template <class T>
        struct Constant {
            T value;
        };

        // A sequence of things that are all pushed onto the stack. Can represent tuples, array literals, and struct initializers.
        struct Tuple {
            std::vector<Expression> elements;
        };

        // An invocation of a function the address of which is visible from the callsite.
        struct Direct_invocation {
            Module_offset           function;
            std::vector<Expression> arguments;
        };

        // An invocation of a function through a pointer the value of which is determined at runtime.
        struct Indirect_invocation {
            bu::Wrapper<Expression> invocable;
            std::vector<Expression> arguments;
        };

        struct Let_binding {
            bu::Wrapper<Expression> initializer;
        };

        struct Loop {
            bu::Wrapper<Expression> body;
        };

        struct Unconditional_jump {
            Local_jump_offset target_offset;
        };

        struct Conditional_jump {
            bu::Wrapper<Expression> condition;
            Local_jump_offset       target_offset;
        };

    }


    struct Expression : std::variant<
        expression::Constant<bu::I8>,
        expression::Constant<bu::I16>,
        expression::Constant<bu::I32>,
        expression::Constant<bu::I64>,
        expression::Constant<bu::U8>,
        expression::Constant<bu::U16>,
        expression::Constant<bu::U32>,
        expression::Constant<bu::U64>,
        expression::Constant<bu::Float>,
        expression::Constant<bu::Char>,
        expression::Constant<bool>,
        expression::Constant<lexer::String>,
        expression::Tuple,
        expression::Direct_invocation,
        expression::Indirect_invocation,
        expression::Let_binding,
        expression::Loop,
        expression::Unconditional_jump,
        expression::Conditional_jump
    > {};


    struct Module {};

}