// builtin_io.cpp
#include "LanClass.h"
#include "LanVariable.h"
#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include <iostream>

static void RegisterIO(MylangeInterpreter& mi, const std::string& scopeId) {

    // nil print (str: o)
    mi.Memory.defineIn(scopeId, LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "print",
            LanFunction::ParamStruct{ { "o", LanType(LanTypeEnum::TypeAny) } },
            [](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (args.empty()) throw std::runtime_error("print requires 1 argument.");
                std::cout << args[0].ToString() << std::endl;
                return std::nullopt;
            }
        )
    ));

	// str input (str: prompt)
    mi.Memory.defineIn(scopeId, LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "input",
            LanFunction::ParamStruct{ { "prompt", LanType(LanTypeEnum::TypeString) } },
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

    // nil dump ()
    mi.Memory.defineIn(scopeId, LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "dump",
            LanFunction::ParamStruct{ },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                mi.Memory.dump();
                return std::nullopt;
            }
        )
    ));

    // nil dumpClass (class: cls)
    mi.Memory.defineIn(scopeId, LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "dumpClass",
            LanFunction::ParamStruct{ { "cls", LanType(LanTypeEnum::TypeClass) } },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                shared_ptr<LanClass> cls = std::get<shared_ptr<LanClass>>(args[0].Value);
                std::cout << "Class: " << cls->Name << "\nMethods:\n";
                for (auto& method : cls->Methods) {
                    std::cout << "\t" << method.first << "/" << method.second->GetId() << "\n";
                }
                std::cout << "Defaulted Properties:\n";
                for (auto& prop : cls->DefaultValues) {
                    std::cout << "\t" << prop.first << " : " << prop.second.Type.ToString() << " => " << prop.second.ToString() << "\n";
                }
                std::cout << "Properties:\n";
                for (auto& prop : cls->Properties) {
                    std::cout << "\t" << prop.first << " : " << prop.second.ToString() << "\n";
                }
                return std::nullopt;
            }
        )
    ));

    // nil dumpClass (str: cls)
    mi.Memory.defineIn(scopeId, LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "dumpClass",
            LanFunction::ParamStruct{ { "clsStr", LanType(LanTypeEnum::TypeString) } },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                shared_ptr<LanClass> cls = mi.ResolveType(std::get<std::string>(args[0].Value)).CustomClass;
                std::cout << "Class: " << cls->Name << "\nMethods:\n";
                for (auto& method : cls->Methods) {
                    std::cout << "\t" << method.first << "/" << method.second->GetId() << "\n";
                }
                std::cout << "Defaulted Properties:\n";
                for (auto& prop : cls->DefaultValues) {
                    std::cout << "\t" << prop.first << " : " << prop.second.Type.ToString() << " => " << prop.second.ToString() << "\n";
                }
                std::cout << "Properties:\n";
                for (auto& prop : cls->Properties) {
                    std::cout << "\t" << prop.first << " : " << prop.second.ToString() << "\n";
                }
                return std::nullopt;
            }
        )
    ));

}

// Register factory
static bool _registered = [] {
    ModuleRegistry::Register("io", RegisterIO);
    return true;
    }();