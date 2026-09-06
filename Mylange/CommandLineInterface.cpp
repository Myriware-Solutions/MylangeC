// IMPORTS //
#include "CommandLineInterface.h"
#include "LanIterableEngine.h"
#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include "Utils.h"
#include <exception>
#include <iostream>
#include <regex>
#include <string>
#include <csignal>
#include <atomic>

std::atomic<bool> interrupted{ false };

void handle_sigint(int signum) {
	interrupted = true;
}

// CODE //
void CommandLineInterface::RunCLI() {
	std::cout << "Mylange Linear Interface Running..." << std::endl
		<< "(c) Myriware Solutions. Version " << CommandLineInterface::Version << std::endl
		<< "To exit, enter 'exit' or 'quit'." << std::endl
		<< "To see CLI variables, enter 'getvars'." << std::endl;

	MylangeInterpreter mi = MylangeInterpreter();
	ModuleRegistry::RegisterHardwires(mi);
	// Line starter in the command line
	mi.Memory.define("CLI_LINE_START", std::make_shared<LanVariable>(LanVariable::String("/> ")));
	mi.Memory.define("CLI_SHOW_R_TYPES", std::make_shared<LanVariable>(LanVariable::Bool(false)));

	std::signal(SIGINT, handle_sigint);
	auto cliv = [&mi](std::string name) {
		return mi.Memory.resolve(name)->Value;
	};
	while (true) {
		// Get input
		std::string input_line_raw;
		CommandLineInterface::PrintOut(std::get<string>(cliv("CLI_LINE_START")), CommandLineInterface::DebugColor::Cyan);

		if (!std::getline(std::cin, input_line_raw)) {
			if (interrupted) {
				std::cout << "\nCaught Ctrl-C, exiting gracefully.\n";
				break;
			}
			// Other fails
			break;
		}

		std::string input_line = std::regex_replace(Utils::TrimString(input_line_raw), std::regex(R"(\s*;\s*$)"), "");
		CommandLineInterface::DebugPrint("Input received: " + input_line);

		if (input_line == "exit" || input_line == "quit") {
			std::cout << "Exiting Mylange CLI." << std::endl;
			break;
		}
		else if (input_line == "clear") {
			std::cout << "\033[2J\033[1;1H";
		}
		else if (input_line == "getvars") {
			// haystack.find(needle) != std::string::npos
			for (auto& [name, var] : mi.Memory.resolveScope("global")->symbols) {
				if (name.find("CLI_") != std::string::npos) {
					CommandLineInterface::Print(std::format("{}/{}:{}", var->Type.ToString(), name, var->ToString()));
				}
			}
		}
		else {
			try
			{
				auto res = mi.InterpretBlock(input_line, true);
				if (res.has_value()) {
					CommandLineInterface::PrintOut(">> ", CommandLineInterface::DebugColor::Magenta);
					if (std::get<bool>(cliv("CLI_SHOW_R_TYPES")))
						CommandLineInterface::Print(std::format("({}) {}", res.value()->Type.ToString(), res.value()->ToString()));
					else
						CommandLineInterface::Print(std::format("{}", res.value()->ToString()));
				}
			}
			catch (const std::exception& e) {
				if (e.what() == "[exit]") {
					std::cout << "Exiting Mylange CLI." << std::endl;
					break;
				}
				CommandLineInterface::Print(std::format("[ERROR] {}", e.what()), CommandLineInterface::DebugColor::Red);
			}
		}
	};
};