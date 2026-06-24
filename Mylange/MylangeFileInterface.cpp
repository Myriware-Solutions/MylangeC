#include <iostream>
#include <string>
#include <regex>
#include <vector>

#include "MylangeFileInterface.h"
#include "Utils.h"
#include "MylangeInterpreter.h"
#include "LanIterableEngine.h"
#include "ModuleRegistry.h"
#include <exception>
//#include "builtin.h"

using namespace std;

regex single_line_comment_pattern(R"(\/\/.*)", std::regex_constants::ECMAScript);
regex multi_line_comment_pattern(R"(\/\[[\s\S]*?\]\/)", std::regex_constants::ECMAScript);
regex newline_whitespace_pattern(R"(\s*\n\s*)", std::regex_constants::ECMAScript);

int FileInterface::InterpretFile(const string& filePath)
{
    std::cout << "Running Mylange script: " << filePath << std::endl;

	string fileContent = Utils::ReadFileContents(filePath);

    
	// std::cout << "File Content:\n" << fileContent << std::endl;
    // Remove all comments
	fileContent = std::regex_replace(fileContent, single_line_comment_pattern, "");
	fileContent = std::regex_replace(fileContent, multi_line_comment_pattern, "");
	fileContent = std::regex_replace(fileContent, newline_whitespace_pattern, " ");

	// cout << "Content without comments:\n" << fileContent << std::endl;

	// Pass in block runner

	auto fu = [&]() {
		MylangeInterpreter mi = MylangeInterpreter();
		ModuleRegistry::RegisterHardwires(mi);
		auto result = mi.InterpretBlock(fileContent);
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
