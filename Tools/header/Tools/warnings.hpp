#ifndef CTP_INCLUDE_WARNINGS_HPP
#define CTP_INCLUDE_WARNINGS_HPP

#ifdef _MSC_VER
#define CTP_WARNING_PUSH _Pragma("warning(push)")
#define CTP_WARNING_POP _Pragma("warning(pop)")
#define CTP_WARNING_WUNDEF _Pragma("warning(disable : 4668)")
#define CTP_WARNING_REDUNDANT_CONSTEVAL_IF
#elif defined __clang__
#define CTP_WARNING_PUSH _Pragma("clang diagnostic push")
#define CTP_WARNING_POP _Pragma("clang diagnostic pop")
#define CTP_WARNING_WUNDEF _Pragma("clang diagnostic ignored \"-Wundef\"")
#define CTP_WARNING_REDUNDANT_CONSTEVAL_IF _Pragma("clang diagnostic ignored \"-Wredundant-consteval-if\"")
#elif defined __GNUC__
#define CTP_WARNING_PUSH _Pragma("GCC diagnostic push")
#define CTP_WARNING_POP _Pragma("GCC diagnostic pop")
#define CTP_WARNING_WUNDEF _Pragma("GCC diagnostic ignored \"-Wundef\"")
#define CTP_WARNING_REDUNDANT_CONSTEVAL_IF
#else
#define CTP_WARNING_PUSH
#define CTP_WARNING_POP
#define CTP_WARNING_WUNDEF
#define CTP_WARNING_REDUNDANT_CONSTEVAL_IF
#endif

#endif // CTP_INCLUDE_WARNINGS_HPP
