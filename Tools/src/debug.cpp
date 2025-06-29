#include <Tools/debug.hpp>

#include <Tools/charconv.hpp>
#include <Tools/windows.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory_resource>

#if CTP_WINDOWS
#include <debugapi.h>
#endif

#if CTP_WINDOWS && CTP_DEBUG
#define SUPPORTS_DEBUGGER_LOGING 1
#else
#define SUPPORTS_DEBUGGER_LOGING 0
#endif

using namespace std::literals;

namespace {
#ifdef NO_CONSOLE_COLOUR
constexpr auto ErrorColour = ""sv;
constexpr auto WarningColour = ""sv;
constexpr auto LogColour = ""sv;
constexpr auto ResetColour = ""sv;
#else
constexpr auto ErrorColour = "\033[31;1m"sv;
constexpr auto WarningColour = "\033[33;1m"sv;
constexpr auto LogColour = "\033[32;1m"sv;
constexpr auto ResetColour = "\033[0m"sv;
#endif // NO_CONSOLE_COLOUR

constexpr auto LogHeader = ""sv;
constexpr auto WarningHeader = "Warn "sv;
constexpr auto ErrorHeader = "ERROR "sv;

#if SUPPORTS_DEBUGGER_LOGING
std::atomic<std::shared_ptr<std::filesystem::path>> CurrentDirectory;
#endif // SUPPORTS_DEBUGGER_LOGING
} // namespace

namespace ctp::debug {
void SetCurrentWorkingDir() CTP_NOEXCEPT_ALLOCS {
#if SUPPORTS_DEBUGGER_LOGING
	std::error_code ec;
	if (auto path = std::filesystem::current_path(ec); !ec) {
		auto newPath = std::make_shared<std::filesystem::path>(std::move(path));
		CurrentDirectory.store(std::move(newPath), std::memory_order_relaxed);
	}
#endif // SUPPORTS_DEBUGGER_LOGING
}

void Log(Stream stream, std::string_view action, std::string_view expr, std::string_view file, int line, std::string_view message) CTP_NOEXCEPT_ALLOCS {

	// TODO: add time to the log header (need clock since app started).
	// TODO: double check if fwrite is the fastest option to use here

	std::array<std::byte, 512> stackMem;
	std::pmr::monotonic_buffer_resource memResource{stackMem.data(), stackMem.size()};
	std::pmr::string out{&memResource};
	out.reserve(stackMem.size());

	// Cut leading directories out of file.
	const auto lastSlash = file.find_last_of("\\/");
	const auto fileName = file.substr(lastSlash == std::string_view::npos ? 0 : lastSlash + 1);

	const ToCharsConverter<int> converter(line);
	const auto lineStr = converter.view();

	std::size_t clickableFilePathOffset = 0;

#if SUPPORTS_DEBUGGER_LOGING
	const bool haveDebugger = IsDebuggerAttached();
	constexpr auto LineNumStart = "("sv;
	constexpr auto LineNumEnd = "): "sv;
	std::string proximateFilePath;
	if (haveDebugger) {
		if (const auto currDir = CurrentDirectory.load(std::memory_order_relaxed)) {
			std::error_code errc;
			// TODO: likely not hard to do myself for my use case and avoid allocs.
			if (auto path = std::filesystem::proximate(file, *currDir, errc); !errc)
				proximateFilePath = path.string();
		}

		// Ofset from clickable file path prefix below.
		const std::size_t fileNameSize = proximateFilePath.size() > 0 ? proximateFilePath.size() : file.size();
		clickableFilePathOffset = fileNameSize + LineNumStart.size() + lineStr.size() + LineNumEnd.size();
	}
#endif // SUPPORTS_DEBUGGER_LOGING

	static constexpr auto headerLen = (std::max)({LogHeader.size(), WarningHeader.size(), ErrorHeader.size()});
	const auto totalSize = clickableFilePathOffset + action.size() + expr.size() + fileName.size() + lineStr.size() + message.size() + headerLen + 16;
	if (stackMem.size() < totalSize)
		out.reserve(totalSize);

#if SUPPORTS_DEBUGGER_LOGING
	// Make clickable file path for debugger.
	if (haveDebugger) {
		out += proximateFilePath.size() > 0 ? proximateFilePath : file;
		out += LineNumStart;
		out += lineStr;
		out += LineNumEnd;
	}
#endif // SUPPORTS_DEBUGGER_LOGING

	auto logStream = &std::cout;
	std::string_view colour;
	switch (stream) {
	case debug::Stream::Log:
		colour = LogColour;
		out += LogHeader;
		break;
	case debug::Stream::Warn:
		logStream = &std::clog;
		colour = WarningColour;
		out += WarningHeader;
		break;
	case debug::Stream::Error:
		logStream = &std::clog;
		colour = ErrorColour;
		out += ErrorHeader;
		break;
	}

	if (!action.empty()) {
		out += action;
		out += ' ';
	}

	if (!expr.empty()) {
		out += '[';
		out += expr;
		out += "] ";
	}

	out += "in "sv;
	out += fileName;
	out += '(';
	out += lineStr;
	out += ')';

	if (!message.empty()) {
		out += ": "sv;
		out += message;
		out += '\n';
	} else {
		out += ".\n"sv;
	}

	logStream->write(colour.data(), colour.size());
	logStream->write(out.data() + clickableFilePathOffset, out.size() - clickableFilePathOffset);
	logStream->write(ResetColour.data(), ResetColour.size());

	if (stream > Stream::Log)
		logStream->flush();

#if SUPPORTS_DEBUGGER_LOGING
	if (!haveDebugger)
		return;

	// Remove the "in filename(lineNum): " redundancy.
	const auto fileNamePos = out.find(fileName, clickableFilePathOffset);
	out.erase(fileNamePos - 3, 3 + fileName.size() + 1 + lineStr.size() + 1 + (message.empty() ? 0 : 2));

#if CTP_WINDOWS
	OutputDebugStringA(out.c_str());
#else
#error "Unimplemented"
#endif
#endif // SUPPORTS_DEBUGGER_LOGING
}

#if CTP_DEBUG

bool IsDebuggerAttached() noexcept {
#if CTP_WINDOWS
	return IsDebuggerPresent() != 0;
#elif CTP_LINUX
#error "TODO: not as straightforward as windows."
#elif CTP_EMSCRIPTEN
	return false;
#else
#error "Not implemented."
#endif
}

void WaitForDebugger() noexcept {
	for (;;) {
		if (IsDebuggerAttached())
			return;

		std::this_thread::sleep_for(20ms);
	}
}

#endif // CTP_DEBUG

} // ctp::debug
