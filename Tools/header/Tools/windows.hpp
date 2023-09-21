#include "config.hpp"

#if CTP_WINDOWS

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4668)
#elif defined __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wundef"
#elif defined __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wundef"
#endif

#include <Windows.h>

#ifdef _MSC_VER
#  pragma warning(pop)
#elif defined __clang__
#  pragma clang diagnostic pop
#elif defined __GNUC__
#  pragma GCC diagnostic pop
#endif

#endif // CTP_WINDOWS
