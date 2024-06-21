#ifndef INCLUDE_CTP_TOOLS_CONFIG_HPP
#define INCLUDE_CTP_TOOLS_CONFIG_HPP

/* -------------------- build ----------------------- */
#if defined DEBUG || defined _DEBUG
#define CTP_DEBUG 1
#define CTP_RELEASE 0
#else
#define CTP_DEBUG 0
#define CTP_RELEASE 1
#endif

/* -------------------- platform -------------------- */

#ifdef __linux__
#define CTP_LINUX 1
#elif defined _WIN32 || defined _WIN64
#define CTP_WINDOWS 1
#elif defined __EMSCRIPTEN__
#define CTP_WASM 1
#else
#error "Unknown platform."
#endif

#ifndef CTP_LINUX
#define CTP_LINUX 0
#endif
#ifndef CTP_WASM
#define CTP_WASM 0
#endif
#ifndef CTP_WINDOWS
#define CTP_WINDOWS 0
#endif

/* -------------------- compiler -------------------- */
#ifdef __clang__

 #if __has_builtin(__builtin_assume)
 #define CTP_ASSUME(expr) __builtin_assume(!!(expr))
 #endif

 #if __has_builtin(__builtin_constant_p)
 #define CTP_IS_CONSTEXPR(expr) __builtin_constant_p(expr)
 #endif

#elif defined __GNUC__

 #define CTP_ASSUME(expr) ((expr) ? static_cast<void>(0) : __builtin_unreachable())
 #if __has_builtin(__builtin_constant_p)
 #define CTP_IS_CONSTEXPR(expr) __builtin_constant_p(expr)
 #endif

#elif defined __EMSCRIPTEN__

 #define CTP_ASSUME(expr)
 #define CTP_BREAK_INTO_DEBUGGER __builtin_trap()

#elif defined _MSC_VER

 #define CTP_ASSUME(expr) __assume(!!(expr))
 #define CTP_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]

 #define CTP_BREAK_INTO_DEBUGGER __debugbreak()

#else
 #error "Unknown compiler."
#endif

/* -------------------- defaults -------------------- */

#ifndef CTP_ASSUME
#define CTP_ASSUME(expr) static_cast<void>((expr) ? 0 : 0)
#endif

#ifndef CTP_IS_CONSTEXPR
#define CTP_IS_CONSTEXPR(expr) 0
#endif


#ifndef CTP_NO_UNIQUE_ADDRESS
#define CTP_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#ifndef CTP_NOEXCEPT_ALLOCS
// Old, stop using this.
#define CTP_NOEXCEPT_ALLOCS noexcept
// Use like noexcept(CTP_NOTHROW_ALLOCS)
#define CTP_NOTHROW_ALLOCS true
#endif

#ifndef CTP_BREAK_INTO_DEBUGGER
 #include <signal.h>
 #ifdef SIGTRAP
  #define CTP_BREAK_INTO_DEBUGGER raise(SIGTRAP)
 #else
  #define CTP_BREAK_INTO_DEBUGGER raise(SIGABRT)
 #endif
#endif

/* --------------------- general --------------------- */

#ifndef CTP_VAR_CONCAT
#define CTP_VAR_CONCAT(x, y) CTP_VAR_CONCAT_INDIRECT(x, y)
#endif
#ifndef  CTP_VAR_CONCAT_INDIRECT
#define CTP_VAR_CONCAT_INDIRECT(x, y) x##y
#endif

#ifndef CTP_IS_CONSTEVAL
#ifdef __cpp_if_consteval
#define CTP_IS_CONSTEVAL consteval
#else
#define CTP_IS_CONSTEVAL (::std::is_constant_evaluated())
#endif
#endif

#ifndef CTP_NOT_CONSTEVAL
#ifdef __cpp_if_consteval
#define CTP_NOT_CONSTEVAL !consteval
#else
#define CTP_NOT_CONSTEVAL (!::std::is_constant_evaluated())
#endif
#endif

#ifndef CTP_NO_EXCEPTIONS
#define CTP_NO_EXCEPTIONS 0
#endif

#if CTP_NO_EXCEPTIONS
#define CTP_USE_EXCEPTIONS 0
#else
#define CTP_USE_EXCEPTIONS 1
#endif

#ifndef CTP_NOEXCEPT
# if CTP_NO_EXCEPTIONS
// Helper for maybe-noexcept functions that is CTP_NO_EXCEPTIONS-aware.
// If exceptions are disabled, will always result in noexcept(true)
# define CTP_NOEXCEPT(...) noexcept
# else
// Helper for maybe-noexcept functions that is CTP_NO_EXCEPTIONS-aware.
// If exceptions are disabled, will always result in noexcept(true)
# define CTP_NOEXCEPT(...) noexcept(__VA_ARGS__)
# endif
#endif

// Use this for commas in macro args.
#define COMMA ,

#endif // INCLUDE_CTP_TOOLS_CONFIG_HPP
