#include <Tools/type_traits.hpp>

namespace ctp {

static_assert(std::is_same_v<add_inner_const_t<int>, const int>);
static_assert(std::is_same_v<add_inner_const_t<int&>, const int&>);
static_assert(std::is_same_v<add_inner_const_t<int*>, const int*>);
static_assert(std::is_same_v<add_inner_const_t<int* const>, const int* const>);

static_assert(std::is_same_v<remove_inner_const_t<const int>, int>);
static_assert(std::is_same_v<remove_inner_const_t<const int&>, int&>);
static_assert(std::is_same_v<remove_inner_const_t<const int*>, int*>);
static_assert(std::is_same_v<remove_inner_const_t<const int* const>, int* const>);

static_assert(has_inner_const_v<const int&>);
static_assert(has_inner_const_v<const int*>);
static_assert(has_inner_const_v<const int* const>);

static_assert(!has_inner_const_v<int&>);
static_assert(!has_inner_const_v<int*>);
static_assert(!has_inner_const_v<int* const>);

} // ctp
