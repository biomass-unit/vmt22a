#include "bu/utilities.hpp"
#include "virtual_machine.hpp"


auto vm::Virtual_machine::serialize() const -> std::vector<std::byte> {
    std::vector<std::byte> buffer;

    auto const write = [&](bu::trivial auto const... args) {
        bu::serialize_to(std::back_inserter(buffer), args...);
    };

    write(version, stack.capacity());

    {
        write(string_buffer.size());
        buffer.insert(
            buffer.end(),
            reinterpret_cast<std::byte const*>(string_buffer.data()),
            reinterpret_cast<std::byte const*>(string_buffer.data() + string_buffer.size())
        );

        write(string_buffer_views.size());
        for (auto const pair : string_buffer_views) {
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


auto vm::Virtual_machine::deserialize(Byte_span bytes) -> Virtual_machine {
    if (auto const extracted_version = extract<bu::Usize>(bytes); version != extracted_version) {
        throw bu::Exception {
            std::format(
                "Attempted to deserialize a virtual machine with "
                "version {}, but the current version is {}",
                extracted_version,
                version
            )
        };
    }

    Virtual_machine machine {
        .stack = bu::Bytestack { extract<bu::Usize>(bytes) }
    };

    {
        auto const string_buffer_size = extract<bu::Usize>(bytes);
        assert(string_buffer_size <= bytes.size());

        machine.string_buffer.assign(
            reinterpret_cast<char const*>(bytes.data()),
            string_buffer_size
        );

        bytes = bytes.subspan(string_buffer_size);
        
        for (auto i = extract<bu::Usize>(bytes); i != 0; --i) {
            machine.string_buffer_views.push_back(extract<bu::Pair<bu::Usize>>(bytes));
        }
    }

    {
        auto const bytecode_size = extract<bu::Usize>(bytes);
        assert(bytecode_size == bytes.size());

        machine.bytecode.bytes.assign(bytes.data(), bytes.data() + bytecode_size);
    }

    return machine;
}