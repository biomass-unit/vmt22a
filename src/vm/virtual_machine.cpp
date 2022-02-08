#include "bu/utilities.hpp"
#include "virtual_machine.hpp"
#include "opcode.hpp"
#include "vm_formatting.hpp"


namespace {

    using VM = vm::Virtual_machine;

    template <bu::trivial T>
    auto push(VM& vm) -> void {
        vm.stack.push(vm.extract_argument<T>());
    }

    template <bool value>
    auto push_bool(VM& vm) -> void {
        vm.stack.push(value);
    }

    template <bu::trivial T>
    auto dup(VM& vm) -> void {
        vm.stack.push(vm.stack.top<T>());
    }

    template <bu::trivial T>
    auto print(VM& vm) -> void {
        bu::print("{}\n", vm.stack.pop<T>());
    }

    template <class T, template <class> class F>
    auto binary_op(VM& vm) -> void {
        auto const a = vm.stack.pop<T>();
        auto const b = vm.stack.pop<T>();
        vm.stack.push(F<T>{}(b, a));
    }

    template <class T> constexpr auto add = binary_op<T, std::plus>;
    template <class T> constexpr auto sub = binary_op<T, std::minus>;
    template <class T> constexpr auto mul = binary_op<T, std::multiplies>;
    template <class T> constexpr auto div = binary_op<T, std::divides>;

    template <class T> constexpr auto eq  = binary_op<T, std::equal_to>;
    template <class T> constexpr auto neq = binary_op<T, std::not_equal_to>;
    template <class T> constexpr auto lt  = binary_op<T, std::less>;
    template <class T> constexpr auto lte = binary_op<T, std::less_equal>;
    template <class T> constexpr auto gt  = binary_op<T, std::greater>;
    template <class T> constexpr auto gte = binary_op<T, std::greater_equal>;

    constexpr auto land = binary_op<bool, std::logical_and>;
    constexpr auto lor  = binary_op<bool, std::logical_or>;

    auto lnand(VM& vm) -> void {
        vm.stack.push(!(vm.stack.pop<bool>() && vm.stack.pop<bool>()));
    }

    auto lnor(VM& vm) -> void {
        vm.stack.push(!(vm.stack.pop<bool>() || vm.stack.pop<bool>()));
    }

    auto lnot(VM& vm) -> void {
        vm.stack.push(!vm.stack.pop<bool>());
    }

    auto jump(VM& vm) -> void {
        vm.jump_to(vm.extract_argument<vm::Jump_offset_type>());
    }

    template <bool value>
    auto jump_bool(VM& vm) -> void {
        auto const offset = vm.extract_argument<vm::Jump_offset_type>();
        if (vm.stack.pop<bool>() == value) {
            vm.jump_to(offset);
        }
    }

    auto call(VM&) -> void {
        bu::unimplemented();
    }

    auto ret(VM&) -> void {
        bu::unimplemented();
    }

    auto iinc_top(VM& vm) -> void {
        vm.stack.push(vm.stack.pop<bu::Isize>() + 1);
    }

    auto halt(VM& vm) -> void {
        vm.keep_running = false;
    }


    constexpr std::array instructions {
        push <bu::Isize>, push <bu::Float>, push <bu::Char>, push_bool<true>, push_bool<false>,
        dup  <bu::Isize>, dup  <bu::Float>, dup  <bu::Char>, dup  <bool>,
        print<bu::Isize>, print<bu::Float>, print<bu::Char>, print<bool>,

        add<bu::Isize>, add<bu::Float>, add<bu::Char>,
        sub<bu::Isize>, sub<bu::Float>, sub<bu::Char>,
        mul<bu::Isize>, mul<bu::Float>, mul<bu::Char>,
        div<bu::Isize>, div<bu::Float>, div<bu::Char>,

        iinc_top,

        eq <bu::Isize>, eq <bu::Float>, eq <bu::Char>, eq <bool>,
        neq<bu::Isize>, neq<bu::Float>, neq<bu::Char>, neq<bool>,
        lt <bu::Isize>, lt <bu::Float>, lt <bu::Char>,
        lte<bu::Isize>, lte<bu::Float>, lte<bu::Char>,
        gt <bu::Isize>, gt <bu::Float>, gt <bu::Char>,
        gte<bu::Isize>, gte<bu::Float>, gte<bu::Char>,

        land, lnand, lor, lnor, lnot,

        jump, jump_bool<true>, jump_bool<false>,

        call, ret,

        halt
    };

    static_assert(instructions.size() == static_cast<bu::Usize>(vm::Opcode::_opcode_count));

}


auto vm::Virtual_machine::run() -> int {
    instruction_pointer = bytecode.bytes.data();
    instruction_anchor = instruction_pointer;
    frame_pointer = stack.base();

    while (keep_running) [[likely]] {
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


auto vm::argument_bytes(Opcode const opcode) noexcept -> bu::Usize {
    static constexpr auto bytecounts = std::to_array<bu::Usize>({
        sizeof(bu::Isize), sizeof(bu::Float), sizeof(bu::Char), 0, 0, // push

        0, 0, 0, 0, // dup
        0, 0, 0, 0, // print

        0, 0, 0, // add
        0, 0, 0, // sub
        0, 0, 0, // mul
        0, 0, 0, // div

        0, // iinc_top

        0, 0, 0, 0, // eq
        0, 0, 0, 0, // neq
        0, 0, 0,    // lt
        0, 0, 0,    // lte
        0, 0, 0,    // gt
        0, 0, 0,    // gte

        0, 0, 0, 0, 0, // logic

        sizeof(Jump_offset_type), sizeof(Jump_offset_type), sizeof(Jump_offset_type), // jump

        sizeof(Jump_offset_type), 0, // call, ret

        0, // halt
    });
    static_assert(bytecounts.size() == static_cast<bu::Usize>(Opcode::_opcode_count));
    return bytecounts[static_cast<bu::Usize>(opcode)];
}