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

#include "CommandLineInterface.h"
#include "MylangeFileInterface.h"

// CODE //
int main(int argc, char* argv[])
{
    std::vector<std::string> args(argv, argv + argc);
    if (argc == 2) {
		FileInterface::InterpretFile(argv[1], std::find(args.begin(), args.end(), "--debug") != args.end());
    }
    else {
        CommandLineInterface::RunCLI();
    }
}

