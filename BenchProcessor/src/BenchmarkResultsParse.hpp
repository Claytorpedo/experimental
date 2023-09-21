#ifndef INCLUDE_CTP_BENCHMARK_PROCESSOR_RESULTS_PARSE_HPP
#define INCLUDE_CTP_BENCHMARK_PROCESSOR_RESULTS_PARSE_HPP

#include <optional>
#include <string_view>

// Parse CSV results of google benchmark.
namespace bench_parse {

std::size_t RecursiveFind(std::string_view str, std::string_view toFind, std::size_t timesToFind);
std::size_t RecursiveFind(std::string_view str, char toFind, std::size_t timesToFind);

struct GetTimeResult {
	std::size_t startPos = 0;
	std::size_t endPos = 0;
	double value = 0;
};

GetTimeResult GetCPUTime(std::string_view line);
void OverwriteCPUTime(std::string& line, double newTime, std::optional<std::pair<std::size_t,std::size_t>> timeStartEnd = {});

struct GetStringResult {
	std::size_t startPos = 0;
	std::string_view value;
};

// Assumes format like "FixtureName/TestName</Arg>"
GetStringResult GetFixtureTestName(std::string_view line);
// Gets the test name suffix, separated by the delimieter, including any range/arg info.
GetStringResult GetFixtureTestNameSuffix(std::string_view line, std::optional<GetStringResult> fixtureTestName = std::nullopt, char delimiter = '_');

} // bench_parse

#endif // INCLUDE_CTP_BENCHMARK_PROCESSOR_RESULTS_PARSE_HPP
