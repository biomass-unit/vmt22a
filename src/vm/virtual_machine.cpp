#include "bu/utilities.hpp"
#include "virtual_machine.hpp"
#include "opcode.hpp"
#include "vm_formatting.hpp"


namespace {

    using VM = vm::Virtual_machine;

    template <bu::trivial T>
    auto push(VM& vm) noexcept -> void {
        vm.stack.push(vm.extract_argument<T>());
    }

    template <bool value>
    auto push_bool(VM& vm) noexcept -> void {
        vm.stack.push(value);
    }

    template <bu::trivial T>
    auto dup(VM& vm) noexcept -> void {
        vm.stack.push(vm.stack.top<T>());
    }

    template <bu::trivial T>
    auto print(VM& vm) noexcept -> void {
        bu::print("{}\n", vm.stack.pop<T>());
    }

    template <class T, template <class> class F>
    auto binary_op(VM& vm) noexcept -> void {
        static_assert(std::same_as<T, std::invoke_result_t<F<T>, T, T>>);
        auto const a = vm.stack.pop<T>();
        auto const b = vm.stack.pop<T>();
        vm.stack.push(F<T>{}(b, a));
    }

    template <class T> constexpr auto add = binary_op<T, std::plus>;
    template <class T> constexpr auto sub = binary_op<T, std::minus>;
    template <class T> constexpr auto mul = binary_op<T, std::multiplies>;
    template <class T> constexpr auto div = binary_op<T, std::divides>;

    auto jump(VM& vm) noexcept -> void {
        vm.jump_to(vm.extract_argument<vm::Jump_offset_type>());
    }

    template <bool value>
    auto jump_bool(VM& vm) noexcept -> void {
        auto const offset = vm.extract_argument<vm::Jump_offset_type>();
        if (vm.stack.pop<bool>() == value) {
            vm.jump_to(offset);
        }
    }

    auto halt(VM& vm) noexcept -> void {
        vm.keep_running = false;
    }


    constexpr std::array instructions {
        push <bu::Isize>, push <bu::Float>, push <char>, push_bool<true>, push_bool<false>,
        dup  <bu::Isize>, dup  <bu::Float>, dup  <char>, dup  <bool>,
        print<bu::Isize>, print<bu::Float>, print<char>, print<bool>,

        add<bu::Isize>, add<bu::Float>, add<char>,
        sub<bu::Isize>, sub<bu::Float>, sub<char>,
        mul<bu::Isize>, mul<bu::Float>, mul<char>,
        div<bu::Isize>, div<bu::Float>, div<char>,

        jump, jump_bool<true>, jump_bool<false>,

        halt
    };

    static_assert(instructions.size() == static_cast<bu::Usize>(vm::Opcode::_opcode_count));

}


auto vm::Virtual_machine::run() -> int {
    instruction_pointer = bytecode.bytes.data();
    instruction_anchor = instruction_pointer;

    while (keep_running) {
        auto const opcode = extract_argument<Opcode>();
        //bu::print("running instruction {}\n", opcode);
        instructions[static_cast<bu::Usize>(opcode)](*this);
    }

    return static_cast<int>(stack.pop<bu::Isize>());
}


auto vm::Virtual_machine::jump_to(Jump_offset_type const offset) noexcept -> void {
    instruction_pointer = instruction_anchor + offset;
}


template <bu::trivial T>
auto vm::Virtual_machine::extract_argument() noexcept -> T {
    T argument;
    std::memcpy(&argument, instruction_pointer, sizeof(T));
    instruction_pointer += sizeof(T);
    return argument;
}