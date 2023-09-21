#include "BenchmarkResultsParse.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std::literals;

namespace {

constexpr auto BaseTimeBenchPrefix = "BaseTime_"sv;

struct BaseTimeInfo {
	std::string line;
	std::string_view suffix;
	double cpuTime = 0;
};

struct FileContents {
	std::vector<std::string> document;
	std::vector<BaseTimeInfo> baseTimeInfos;
	std::optional<std::size_t> labelFieldIndex;
};

[[nodiscard]] FileContents ParseFile(const std::string& fileName) {
	std::ifstream file{fileName};
	if (!file.is_open()) {
		std::cerr << "Failed to open file [" << fileName << "].\n";
		std::terminate();
	}

	FileContents fileContents;

	fileContents.document.reserve(100);
	fileContents.baseTimeInfos.reserve(20);

	std::string line;
	while (file >> line) {
		if (line.empty())
			continue;

		// Quick-and-dirty check for a test case label.
		if (line[0] == '\"') {
			const auto [pos, testName] = bench_parse::GetFixtureTestName(line);
			if (testName.starts_with(BaseTimeBenchPrefix)) {
				auto& info = fileContents.baseTimeInfos.emplace_back();
				info.line = std::move(line);
				info.suffix = bench_parse::GetFixtureTestNameSuffix(info.line).value;
				info.cpuTime = bench_parse::GetCPUTime(info.line).value;
			}
		} else if (const auto pos = line.find("label"sv); pos != std::string::npos) {
			fileContents.labelFieldIndex = std::count(line.begin(), line.begin() + pos, ',');
		}

		if (!line.empty())
			fileContents.document.push_back(std::move(line));
	}

	return fileContents;
}

void SetTestMetaData(std::string& line, const std::string_view testName, const std::string_view testNameSuffix, const std::optional<std::size_t> labelField) {
	bool labelFieldSet = false;
	std::size_t labelPos;
	if (labelField) {
		labelPos = bench_parse::RecursiveFind(line, ',', *labelField);

		if (labelPos == std::string::npos) {
			std::cerr << "Document is corrupted, or does not have correct format (see line [" << line << "]).\n";
			std::terminate();
		}

		++labelPos; // Skip the comma.

		labelFieldSet = labelPos != line.size() && line[labelPos] != ',';
	} else {
		line += ',';
		labelPos = line.size();
	}

	// The category is the test name suffix without any arg info.
	auto category_view = testName;
	while (!testNameSuffix.starts_with(category_view))
		category_view.remove_prefix(1);

	// Copy view data before potentially reallocating line.
	const auto category = std::string{category_view};
	const auto label = std::string{testName.substr(0, testName.size() - category.size() - 1)};

	line.reserve(line.size() + testName.size() * 2);

	if (!labelFieldSet) {
		line.insert(labelPos, label);
	}

	line += ',';
	line.append(category);
}

void ProcessFile(FileContents& fileContents) {
	auto& baseTimeInfos = fileContents.baseTimeInfos;

	// Subtract any matching base-times from test times. Add label field if it doesn't exist, and category field.
	for (auto& line : fileContents.document) {
		if (line[0] != '\"') {
			if (!fileContents.labelFieldIndex)
				line.append(",label"sv);
			line.append(",category"sv);
			continue;
		}

		std::decay_t<decltype(baseTimeInfos)>::const_iterator it;

		// Modify line with metadata.
		{
			const auto testName = bench_parse::GetFixtureTestName(line);
			const auto testNameSuffix = bench_parse::GetFixtureTestNameSuffix(line, testName).value;

			if (testNameSuffix.empty())
				continue;

			it = std::ranges::find(baseTimeInfos, testNameSuffix, &BaseTimeInfo::suffix);

			SetTestMetaData(line, testName.value, testNameSuffix, fileContents.labelFieldIndex);
		}

		if (it == baseTimeInfos.end())
			continue;

		const BaseTimeInfo& BaseTimeInfo = *it;

		const auto lineTimeInfo = bench_parse::GetCPUTime(line);

		const auto adjustedTime = lineTimeInfo.value - BaseTimeInfo.cpuTime;

		bench_parse::OverwriteCPUTime(line, adjustedTime, std::make_pair(lineTimeInfo.startPos, lineTimeInfo.endPos));
	}
}

} // namespace

int main(int argc, char* argv[])
{
	std::string fileName;
	if (argc >= 2) {
		fileName = argv[1];
	} else {
		std::cout << "Provide file name: ";
		std::cin >> fileName;
	}

	auto fileContents = ParseFile(fileName);

	ProcessFile(fileContents);

	if (argc > 2) {
		fileName = argv[2];
	} else {
		if (const auto extPos = fileName.rfind('.'); extPos != std::string::npos)
			fileName.erase(extPos, fileName.size() - extPos);
		fileName.append("_processed.csv"sv);
	}

	std::ofstream out{fileName, std::ios::out | std::ios::trunc};

	if (!out.is_open()) {
		for (const auto& line : fileContents.document)
			std::cout << line << '\n';

		std::cerr << "Failed to open out file [" << fileName << "]\n.";
		return 3;
	}

	for (const auto& line : fileContents.document) {
		std::cout << line << '\n';
		out << line << '\n';
	}

	return 0;
}
