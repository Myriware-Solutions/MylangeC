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

int FileInterface::InterpretFile(const string& filePath)
{
    std::cout << "Running Mylange script: " << filePath << std::endl;

	

	auto fu = [&]() {
		MylangeInterpreter mi = MylangeInterpreter();
		ModuleRegistry::RegisterHardwires(mi);
		auto result = mi.InterpretBlock(FileInterface::CleanFile(filePath));
		if (result.has_value())
			cout << "Program exited with value: (" + result.value().Type.ToString() + ") " + result.value().ToString();
		else
			std::cout << "Program exited with no return value." << std::endl;
	};

	try {
		fu();
	}
	catch (const exception& e) {
		cerr << "Error during interpretation: " << e.what() << endl;
		return 1;
	}
    return 0;
}
