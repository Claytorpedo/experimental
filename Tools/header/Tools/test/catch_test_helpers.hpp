#ifndef INCLUDE_CTP_TOOLS_TEST_CATCH_TEST_HELPERS_HPP
#define INCLUDE_CTP_TOOLS_TEST_CATCH_TEST_HELPERS_HPP

#include <Tools/config.hpp>
#include <Tools/warnings.hpp>
#include <Tools/debug.hpp>

#include <catch2/catch_test_macros.hpp>

#ifndef CTP_CONSTEXPR_ASSERTS_ENABLED
#define CTP_CONSTEXPR_ASSERTS_ENABLED 1
#endif

// Check macro that does a catch2 assert at runtime but is constexpr friendly.
#define CTP_CHECK(...) do { \
  CTP_WARNING_PUSH \
  CTP_WARNING_REDUNDANT_CONSTEVAL_IF \
	if CTP_IS_CONSTEVAL{ \
		if constexpr (CTP_CONSTEXPR_ASSERTS_ENABLED) \
			ctpAssert(__VA_ARGS__); \
	} else { \
		CHECK(__VA_ARGS__); \
	} \
  CTP_WARNING_POP \
} while(0)

// Check macro that does a catch2 assert at runtime but is constexpr friendly.
#define CTP_CHECK_FALSE(...) do { \
  CTP_WARNING_PUSH \
  CTP_WARNING_REDUNDANT_CONSTEVAL_IF \
	if CTP_IS_CONSTEVAL{ \
		if constexpr (CTP_CONSTEXPR_ASSERTS_ENABLED) \
			ctpAssert(!static_cast<bool>(__VA_ARGS__)); \
	} else { \
		CHECK_FALSE(__VA_ARGS__); \
	} \
  CTP_WARNING_POP \
} while(0)

#ifndef DISABLE_CONSTEXPR_TESTS
#define TEST_CONSTEXPR [[maybe_unused]] static constexpr
#else
#define TEST_CONSTEXPR [[maybe_unused]]
#endif

#endif //INCLUDE_CTP_TOOLS_TEST_CATCH_TEST_HELPERS_HPP
