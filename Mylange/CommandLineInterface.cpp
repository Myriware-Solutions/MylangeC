// IMPORTS //
#include <iostream>
#include <string>
#include <regex>
#include <exception>

#include "CommandLineInterface.h"
#include "MylangeInterpreter.h"
#include "Utils.h"
#include "LanIterableEngine.h"
#include "ModuleRegistry.h"

// CODE //

using namespace std;

void CommandLineInterface::RunCLI() {
	cout << "Mylange Linear Interface Running..." << endl;

	MylangeInterpreter mi = MylangeInterpreter();
	ModuleRegistry::RegisterHardwires(mi);
	// Line starter in the command line
	mi.Memory.define("LINE_START", LanVariable(LanType(LanTypeEnum::TypeString), "/>"));

	while (true) {
		string input_line_raw;
		cout << std::get<string>(mi.Memory.resolve("LINE_START")->Value);
		getline(cin, input_line_raw);

		string input_line = regex_replace(Utils::TrimString(input_line_raw), regex(R"(\s*;\s*$)"), "");
		CommandLineInterface::DebugPrint("Input received: " + input_line);

		if (input_line == "exit" || input_line == "quit") {
			cout << "Exiting Mylange CLI." << endl;
			break;
		}

		try
		{
			auto res = mi.InterpretBlock(input_line, true);
			if (res.has_value()) {
				std::cout << "<< (" << res.value().Type.ToString() << ") " << res.value().ToString() << std::endl;
			}
		}
		catch (const exception& e) {
			if (e.what() == "[exit]") {
				std::cout << "Exiting Mylange CLI." << std::endl;
				break;
			}
			std::cout << "[ERROR] " << e.what() << ";" << endl;
		}
		

	};
}

void CommandLineInterface::DebugPrint(const std::string& message, const int& indent)
{
	if (!CommandLineInterface::DebugEnabled) return;
	string indent_str = string(indent * 4, ' ');
	try {
		cout << "[DEBUG] " << indent_str << message << endl;
	}
	catch (const exception& e) {
		cout << "[OUTERR] error outputting: " << e.what() << endl;
	};
};