/*
Welcome to the entry point of the Mylange programming language interpreter.
This file contains the main function that initializes the interpreter,
processes command-line arguments, and either runs a file or starts an interactive CLI.

Developed by Myriware Solutions, 2026.

"Mylange, it rhymes with orange."
*/

// IMPORTS //
#include <string>
#include <vector>
#include <regex>
#include <algorithm>

#include "CommandLineInterface.h"
#include "MylangeFileInterface.h"

// CODE //

const std::regex valid_arg_pattern = std::regex(R"(^\s*--\w+)");

const std::vector<std::string> valid_args = {
    "--debug"
};

bool find_a(const std::vector<std::string>& haystack, const std::string& needle) {
    if (std::find(haystack.begin(), haystack.end(), needle) != haystack.end()) return true;
    return false;
}

int main(int argc, char* argv[])
{
    std::vector<std::string> args(argv, argv + argc);
	CommandLineInterface::Args = args;
    for (auto& arg : CommandLineInterface::Args) {
        if (arg == "--debug") CommandLineInterface::DebugEnabled = true;
    }

    if (argc > 1 && !std::regex_match(argv[1], valid_arg_pattern)) {
		auto result = FileInterface::InterpretFile(argv[1]);
        if (result.has_value())
            cout << "Program exited with value: (" + result.value().Type.ToString() + ") " + result.value().ToString();
        else
            std::cout << "Program exited with no return value." << std::endl;
    }
    else {
        CommandLineInterface::RunCLI();
    }
}

