#include "BenchmarkResultsParse.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <iostream>

using namespace std::literals;

namespace bench_parse {

std::size_t RecursiveFind(std::string_view str, std::string_view toFind, std::size_t timesToFind) {
	std::size_t pos = 0;
	for (; (timesToFind > 0) && (pos != std::string_view::npos) && (pos + 1 < str.size()); --timesToFind)
		pos = str.find(toFind, pos + 1);
	return pos;
}
std::size_t RecursiveFind(std::string_view str, char toFind, std::size_t timesToFind) {
	return RecursiveFind(str, std::string_view{&toFind, 1}, timesToFind);
}

GetTimeResult GetCPUTime(const std::string_view line) {
	GetTimeResult result;

	// real_time is the 3rd field.
	auto startComma = RecursiveFind(line, ","sv, 3);
	if (startComma == std::string_view::npos) {
		std::cerr << "GetCPUTime - Didn't find start comma.\n";
		return result;
	}
	++startComma; // Skip comma.

	const auto endComma = line.find(',', startComma);
	if (endComma == std::string_view::npos) {
		std::cerr << "GetCPUTime - Didn't find end comma.\n";
		return result;
	}

	double time = 0;
	const auto [ptr, ec] = std::from_chars(line.data() + startComma, line.data() + endComma, time);
	if (ec == std::errc{}) {
		result.startPos = startComma;
		result.endPos = endComma;
		result.value = time;
		return result;
	}

	std::cerr << "GetCPUTime - Didn't find start comma.\n";
	return result;
}

void OverwriteCPUTime(std::string& line, double newTime, std::optional<std::pair<std::size_t, std::size_t>> timeStartEnd) {
	std::size_t currentStart = 0;
	std::size_t currentEnd = 0;
	if (timeStartEnd) {
		std::tie(currentStart, currentEnd) = *timeStartEnd;
	} else {
		const auto info = GetCPUTime(line);
		currentStart = info.startPos;
		currentEnd = info.endPos;
	}

	line.erase(currentStart, currentEnd - currentStart);

	std::array<char, 100> chars;
	auto [ptr, ec] = std::to_chars(chars.data(), chars.data() + 100, newTime);

	if (ec != std::errc{}) {
		std::cerr << "OverwriteCPUTime - value too large.\n";
		return;
	}

	const auto str = std::string_view{chars.data(), ptr};

	line.insert(currentStart, str);
}

GetStringResult GetFixtureTestName(const std::string_view line) {
	GetStringResult result;

	auto nameStart = line.find('/');
	if (nameStart == std::string_view::npos) {
		std::cerr << "GetFixtureTestName - Didn't find start slash.\n";
		return result;
	}
	++nameStart; // Skip slash.

	const auto nameEnd = line.find_first_of("/\"", nameStart);
	if (nameStart == std::string_view::npos) {
		std::cerr << "GetFixtureTestName - Didn't find end slash or quote.\n";
		return result;
	}

	result.startPos = nameStart;
	result.value = line.substr(nameStart, nameEnd - nameStart);
	return result;
}

GetStringResult GetFixtureTestNameSuffix(const std::string_view line, std::optional<GetStringResult> fixtureTestName, const char delimiter) {
	if (!fixtureTestName)
		fixtureTestName = GetFixtureTestName(line);

	GetStringResult result;

	if (const std::size_t delimPos = fixtureTestName->value.rfind(delimiter); delimPos != std::string_view::npos) {
		result.startPos = fixtureTestName->startPos + delimPos + 1;
	} else {
		result.startPos = fixtureTestName->startPos;
	}

	// Include any range/arg info.
	auto testNameEnd = line.find('\"', result.startPos);

	if (testNameEnd == std::string_view::npos) [[unlikely]]
		testNameEnd = fixtureTestName->startPos + fixtureTestName->value.size();

	result.value = line.substr(result.startPos, testNameEnd - result.startPos);
	return result;
}

} // bench_parse
