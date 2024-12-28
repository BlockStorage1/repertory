#ifndef LIBREPERTORY_INCLUDE_VERSION_HPP_
#define LIBREPERTORY_INCLUDE_VERSION_HPP_

#include <string_view>

namespace repertory {
[[nodiscard]] auto project_get_git_rev() -> std::string_view;

[[nodiscard]] auto project_get_version() -> std::string_view;
} // namespace repertory

#endif // LIBREPERTORY_INCLUDE_VERSION_HPP_
