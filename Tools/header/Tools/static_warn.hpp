#ifndef CTP_INCLUDE_STATIC_WARN_HPP
#define CTP_INCLUDE_STATIC_WARN_HPP

#ifndef STATIC_WARN_ENABLED
#define STATIC_WARN_ENABLED 1
#endif

#if STATIC_WARN_ENABLED

#ifdef __clang__
#define static_warn_do_fail \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic warning \"-Wdeprecated-declarations\"") \
	[[maybe_unused]] static_warn_failure _fail{}; \
	_Pragma("clang diagnostic pop")
	
#elif defined _MSC_VER
#define static_warn_do_fail \
	_Pragma("warning(push)") \
	_Pragma("warning(1 : 4996)") \
	[[maybe_unused]] static_warn_failure _fail{}; \
	_Pragma("warning(pop)")
#else
#define static_warn_do_fail [[maybe_unused]] static_warn_failure _fail{};
#endif

// static_warn macro provides a custom warning message at compile time
// if a static condition is not met.
#define static_warn(expression, message) [] () consteval { \
	if constexpr (!static_cast<bool>(expression)) { \
		struct [[deprecated(message)]] static_warn_failure {}; \
		static_warn_do_fail \
	} \
}()

#else
#define static_warn(expression, message) []() consteval {} ()
#endif //STATIC_WARN_ENABLED
#endif //CTP_INCLUDE_STATIC_WARN_HPP
