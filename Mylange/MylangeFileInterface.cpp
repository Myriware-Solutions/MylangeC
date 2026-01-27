#include <iostream>
#include <string>
#include <regex>
#include <vector>

#include "MylangeFileInterface.h"
#include "Utils.h"
#include "MylangeInterpreter.h"
#include "LanIterableEngine.h"

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
	fileContent = std::regex_replace(fileContent, newline_whitespace_pattern, "");

	// cout << "Content without comments:\n" << fileContent << std::endl;

	// Pass in block runner

	try {
		MylangeInterpreter mi = MylangeInterpreter();
		auto result = mi.InterpretBlock("global", fileContent).value_or(make_unique<LanVariable>());
		cout << "Program exited with value: (" + result->Type.ToString() + ") " + result->ToString();
	}
	catch (const exception& e) {
		cerr << "Error during interpretation: " << e.what() << endl;
		return 1;
	}
    return 0;
}
