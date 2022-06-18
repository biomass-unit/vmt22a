#include "bu/utilities.hpp"
#include "language/configuration.hpp"
#include "virtual_machine.hpp"


auto vm::Executable_program::serialize() const -> std::vector<std::byte> {
    std::vector<std::byte> buffer;

    auto const write = [&](bu::trivial auto const... args) {
        bu::serialize_to(std::back_inserter(buffer), args...);
    };

    write(language::version, stack_capacity);

    {
        write(constants.string_buffer.size());
        buffer.insert(
            buffer.end(),
            reinterpret_cast<std::byte const*>(constants.string_buffer.data()),
            reinterpret_cast<std::byte const*>(constants.string_buffer.data() + constants.string_buffer.size())
        );

        write(constants.string_buffer_views.size());
        for (auto const pair : constants.string_buffer_views) {
            write(pair);
        }
    }

    {
        write(bytecode.bytes.size());
        buffer.insert(buffer.end(), bytecode.bytes.begin(), bytecode.bytes.end());
    }

    return buffer;
}


namespace {

    using Byte_span = std::span<std::byte const>;

    template <bu::trivial T>
    auto extract(Byte_span& span) -> T {
        assert(span.size() >= sizeof(T));

        T value;
        std::memcpy(&value, span.data(), sizeof(T));
        span = span.subspan(sizeof(T));
        return value;
    }

}


auto vm::Executable_program::deserialize(Byte_span bytes) -> Executable_program {
    if (auto const extracted_version = extract<bu::Usize>(bytes); language::version != extracted_version) {
        throw bu::exception(
            "Attempted to deserialize a program compiled with "
            "vmt22a version {}, but the current version is {}",
            extracted_version,
            language::version
        );
    }

    Executable_program program {
        .stack_capacity = extract<bu::Usize>(bytes)
    };

    {
        auto const string_buffer_size = extract<bu::Usize>(bytes);
        bu::always_assert(string_buffer_size <= bytes.size());

        program.constants.string_buffer.assign(
            reinterpret_cast<char const*>(bytes.data()),
            string_buffer_size
        );

        bytes = bytes.subspan(string_buffer_size);
        
        for (auto i = extract<bu::Usize>(bytes); i != 0; --i) {
            program.constants.string_buffer_views.push_back(extract<bu::Pair<bu::Usize>>(bytes));
        }
    }

    {
        auto const bytecode_size = extract<bu::Usize>(bytes);
        bu::always_assert(bytecode_size == bytes.size());

        program.bytecode.bytes.assign(bytes.data(), bytes.data() + bytecode_size);
    }

    return program;
}