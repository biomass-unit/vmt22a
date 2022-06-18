#include "bu/utilities.hpp"
#include "dependency.hpp"


auto dependency::Module_interface::is_recompilation_necessary() const -> bool {
    return !std::filesystem::exists(module_path)
        || last_write_time != std::filesystem::last_write_time(module_path)
        || std::ranges::any_of(imports, &Module_interface::is_recompilation_necessary);
}


auto dependency::Module_interface::serialize() const
    -> std::vector<std::byte>
{
    bu::unimplemented();
}

auto dependency::Module_interface::deserialize(std::span<std::byte const>)
    -> Module_interface
{
    bu::unimplemented();
}