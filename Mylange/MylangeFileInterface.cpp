#include "LanIterableEngine.h"
#include "ModuleRegistry.h"
#include "MylangeFileInterface.h"
#include "MylangeInterpreter.h"
#include "Utils.h"
#include <exception>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

using namespace std;

regex single_line_comment_pattern(R"(\/\/.*)", std::regex_constants::ECMAScript);
regex multi_line_comment_pattern(R"(\/\[[\s\S]*?\]\/)", std::regex_constants::ECMAScript);
regex newline_whitespace_pattern(R"(\s*\n\s*)", std::regex_constants::ECMAScript);

std::string FileInterface::CleanFile(const string& filePath) {
	string fileContent = Utils::ReadFileContents(filePath);

	// Remove all comments
	fileContent = std::regex_replace(fileContent, single_line_comment_pattern, "");
	fileContent = std::regex_replace(fileContent, multi_line_comment_pattern, "");
	fileContent = std::regex_replace(fileContent, newline_whitespace_pattern, " ");

	return fileContent;
}

std::optional<LanVariable> FileInterface::InterpretFile(const string& filePath, bool ignoreMessage)
{
    if (!ignoreMessage) std::cout << "Running Mylange script: " << filePath << std::endl;

	

	auto fu = [&]() {
		MylangeInterpreter mi = MylangeInterpreter();
		ModuleRegistry::RegisterHardwires(mi);
		auto result = mi.InterpretBlock(FileInterface::CleanFile(filePath));
		return result;
	};

	//return fu();

	try {
		return fu();
	}
	catch (const exception& e) {
		cerr << "Error during interpretation: " << e.what() << endl;
		return nullopt;
	}
    return nullopt;
}
