#include "bu/utilities.hpp"
#include "dependency.hpp"


auto dependency::Module_interface::Visible_components::hash() const -> bu::Usize {
    bu::todo();
}

auto dependency::Module_interface::Invisible_components::hash() const -> bu::Usize {
    bu::todo();
}


auto dependency::Module_interface::serialize() const -> std::vector<std::byte> {
    bu::todo();
}

auto dependency::Module_interface::deserialize(std::span<std::byte const>) -> Module_interface {
    bu::todo();
}