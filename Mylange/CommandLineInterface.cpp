// IMPORTS //
#include <iostream>
#include <string>
#include <regex>

#include "CommandLineInterface.h"
#include "MylangeInterpreter.h"
#include "Utils.h"

// CODE //

using namespace std;

void CommandLineInterface::RunCLI() {
	cout << "Mylange Linear Interface Running..." << endl;

	MylangeInterpreter mi = MylangeInterpreter();

	while (true) {
		string input_line_raw;
		cout << "#! ";
		getline(cin, input_line_raw);

		string input_line = regex_replace(Utils::TrimString(input_line_raw), regex(R"(\s*;\s*$)"), "");
		CommandLineInterface::DebugPrint("Input received: " + input_line);

		if (input_line == "*vars")
		{
			for (const auto& pair : mi.MemBook.Variables)
			{
				CommandLineInterface::DebugPrint(pair.first + "|" + pair.second.Type.ToString() + "|" + pair.second.ToString());
			}
		}

		try
		{
			mi.Interpret("global", input_line);
		}
		catch (const exception& e) {
			cout << "[ERROR] " << e.what() << endl;
		}
		

	};
}

void CommandLineInterface::DebugPrint(const std::string& message)
{
	cout << "[DEBUG] " << message << endl;
};