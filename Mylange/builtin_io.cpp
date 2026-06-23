// builtin_io.cpp
#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include "LanVariable.h"
#include <iostream>

static void RegisterIO(MylangeInterpreter& mi, const std::string& scopeId) {

    // print (str)
    mi.Memory.defineIn(scopeId, LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "print",
            std::map<std::string, LanType>{ { "x", LanType(LanTypeEnum::TypeString) } },
            [](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (args.empty()) throw std::runtime_error("print requires 1 argument.");
                std::cout << args[0].ToString() << std::endl;
                return std::nullopt;
            }
        )
    ));

	// input (str) -> str
    mi.Memory.defineIn(scopeId, LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "input",
            std::map<std::string, LanType>{ { "prompt", LanType(LanTypeEnum::TypeString) } },
            [](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (!args.empty())
                    std::cout << std::get<std::string>(args[0].Value);
                std::string line;
                std::getline(std::cin, line);
				CommandLineInterface::DebugPrint("Input received: " + line);
                return LanVariable(LanType(LanTypeEnum::TypeString), line);
            }
        )
    ));
}

// This runs at program startup — registers the factory, NOT the functions
static bool _registered = [] {
    ModuleRegistry::Register("io", RegisterIO);
    return true;
    }();