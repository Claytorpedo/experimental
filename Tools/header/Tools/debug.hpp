#ifndef INCLUDE_CTP_TOOLS_DEBUG_HPP
#define INCLUDE_CTP_TOOLS_DEBUG_HPP

#include "config.hpp"


// TODO: option to keep some logs in Release mode

#if CTP_DEBUG
#include <string_view>

namespace ctp::debug {
enum class Stream {
	Log,
	Warn,
	Error
};

void SetCurrentWorkingDir() CTP_NOEXCEPT_ALLOCS;
void Log(Stream stream, std::string_view action, std::string_view expr, std::string_view file, int line, std::string_view message = {}) CTP_NOEXCEPT_ALLOCS;

constexpr void BreakMsg(Stream stream, std::string_view file, int line, std::string_view message) noexcept {
	if CTP_NOT_CONSTEVAL {
		::ctp::debug::Log(stream, "debug break", "", file, line, message);
		CTP_BREAK_INTO_DEBUGGER;
	}
}

namespace detail {
// If the compiler fails here, you have hit an assert or ctpFail in a constexpr context.
inline void ConstexprAssertFailed() {}
} // detail

constexpr void Fail(std::string_view msg) noexcept {
	if CTP_IS_CONSTEVAL {
		// Conditional to postpone call to non-constexpr failure function.
		false ? void(0) : detail::ConstexprAssertFailed();
	} else {
		::ctp::debug::Log(ctp::debug::Stream::Error, "program failure", "", __FILE__, __LINE__, msg);
		CTP_BREAK_INTO_DEBUGGER;
		::std::terminate();
	}
}

constexpr void AssertFail(std::string_view expr, std::string_view msg) noexcept {
	if CTP_IS_CONSTEVAL {
		// Conditional to postpone call to non-constexpr failure function.
		false ? void(0) : detail::ConstexprAssertFailed();
	} else {
		::ctp::debug::Log(ctp::debug::Stream::Error, "assertion failed", expr, __FILE__, __LINE__, msg);
		CTP_BREAK_INTO_DEBUGGER;
	}
}

bool IsDebuggerAttached() noexcept;
void WaitForDebugger() noexcept;

} // namespace ctp::debug

// Initialize the default logger.
#define ctpInitDebugLogging() do { \
	::ctp::debug::SetCurrentWorkingDir(); \
} while (0)

// Assert a condition is true.
#define ctpAssertMsg(expression, message) do { \
	if (!static_cast<bool>(expression)) [[unlikely]] \
		::ctp::debug::AssertFail(#expression, message); \
} while(0)

// Indicate a failure state.
#define ctpFailMsg(message) do { ::ctp::debug::Fail(message); } while(0)

// Break into the debugger. Ignored in constexpr contexts.
#define ctpBreak() do { \
	if CTP_NOT_CONSTEVAL \
		CTP_BREAK_INTO_DEBUGGER; \
} while(0)
// Break into the debugger. Ignored in constexpr contexts.
#define ctpBreakMsgStream(stream, message) do { ::ctp::debug::BreakMsg(stream, message); } while(0)

// Break into the debugger if one is detected. Ignored in constexpr contexts.
#define ctpBreakIfDebuggerAttached()  do { \
	if CTP_NOT_CONSTEVAL \
		if (::ctp::debug::IsDebuggerAttached()) \
			CTP_BREAK_INTO_DEBUGGER; \
} while(0)

// Log a message to the default logger. Ignored in constexpr contexts.
#define ctpLogStream(stream, message) do { ::ctp::debug::Log(stream, "", "", __FILE__, __LINE__, message); } while (0)

// Log a message to the default logger if a condition is true.
#define ctpLogIfStream(stream, expression, message) do { \
	if (!static_cast<bool>(expression)) \
		::ctp::debug::Log(stream, "condition met", #expression, __FILE__, __LINE__, message); \
} while(0)

#else
// Disabled, noop macro definitions.

// Initialize the default logger.
#define ctpInitDebugLogging() do {} while(0)
// Assert a condition is true.
#define ctpAssertMsg(expression, message) do {} while(0)
// Indicate a failure state.
#define ctpFailMsg(message) do {} while(0)
// Break into the debugger. Ignored in constexpr contexts.
#define ctpBreak() do {} while(0)
// Break into the debugger. Ignored in constexpr contexts.
#define ctpBreakMsgStream(stream, message) do {} while(0)
// Break into the debugger if one is detected. Ignored in constexpr contexts.
#define ctpBreakIfDebuggerAttached() do {} while(0)
// Log a message to the default logger. Ignored in constexpr contexts.
#define ctpLogStream(stream, message)  do {} while(0)
// Log a message to the default logger if a condition is true.
#define ctpLogIfStream(stream, expression, message) do {} while(0)
#endif // CTP_DEBUG

// Expect a precondition is true.
#define ctpExpects(...) ctpAssert(__VA_ARGS__)
// Assert a condition is true.
#define ctpAssert(...) ctpAssertMsg(__VA_ARGS__, {})
// Ensure a postcondition is true.
#define ctpEnsures(...) ctpAssert(__VA_ARGS__)
// Indicate a failure state.
#define ctpFail() ctpFailMsg({})

// Break into the debugger. Ignored in constexpr contexts.
#define ctpBreakMsgi(message) ctpBreakMsgStream(ctp::debug::Stream::Log, message)
// Break into the debugger. Ignored in constexpr contexts.
#define ctpBreakMsgw(message) ctpBreakMsgStream(ctp::debug::Stream::Warn, message)
// Break into the debugger. Ignored in constexpr contexts.
#define ctpBreakMsge(message) ctpBreakMsgStream(ctp::debug::Stream::Error, message)
// Break into the debugger. Ignored in constexpr contexts.
#define ctpBreakMsg(message) ctpBreakMsgw(message)

// Log a message to the default logger. Ignored in constexpr contexts.
#define ctpLogi(message) ctpLogStream(::ctp::debug::Stream::Log, message)
// Log a message to the default logger. Ignored in constexpr contexts.
#define ctpLogw(message) ctpLogStream(::ctp::debug::Stream::Warn, message)
// Log a message to the default logger. Ignored in constexpr contexts.
#define ctpLoge(message) ctpLogStream(::ctp::debug::Stream::Error, message)
// Log a message to the default logger. Ignored in constexpr contexts.
#define ctpLog(message) ctpLogi(message)

// Log a message to the default logger if a condition is true.
#define ctpLogIfi(expression, message) ctpLogIfStream(::ctp::debug::Stream::Log, expression, message)
// Log a message to the default logger if a condition is true.
#define ctpLogIfw(expression, message) ctpLogIfStream(::ctp::debug::Stream::Warn, expression, message)
// Log a message to the default logger if a condition is true.
#define ctpLogIfe(expression, message) ctpLogIfStream(::ctp::debug::Stream::Error, expression, message)
// Log to the default logger if a condition is true.
#define ctpLogIf(expression) ctpLogIfStream(::ctp::debug::Stream::Log, expression, {})

#endif // INCLUDE_CTP_TOOLS_DEBUG_HPP
