#ifndef LIBREPERTORY_INCLUDE_INITIALIZE_HPP_
#define LIBREPERTORY_INCLUDE_INITIALIZE_HPP_

namespace repertory {
void project_cleanup();

[[nodiscard]] auto project_initialize() -> bool;
} // namespace repertory

#endif // LIBREPERTORY_INCLUDE_INITIALIZE_HPP_
