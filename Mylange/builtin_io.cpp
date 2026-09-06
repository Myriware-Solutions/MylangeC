// builtin_io.cpp
#include "LanClass.h"
#include "LanVariable.h"
#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include <iostream>

static void RegisterIO(MylangeInterpreter& mi, const std::string& scopeId) {

    // nil print (str: o)
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "print",
            LanFunction::ParamStruct{ { "o", LanType(LanTypeEnum::TypeAny) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (args.empty()) throw std::runtime_error("print requires 1 argument.");
                std::cout << args[0]->ToString() << std::endl;
                return std::nullopt;
            }
        )
    ));

	// str input (str: prompt)
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "input",
            LanFunction::ParamStruct{ { "prompt", LanType(LanTypeEnum::TypeString) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (!args.empty())
                    std::cout << std::get<std::string>(args[0]->Value);
                std::string line;
                std::getline(std::cin, line);
				CommandLineInterface::DebugPrint("Input received: " + line);
                return std::make_shared<LanVariable>(LanType(LanTypeEnum::TypeString), line);
            }
        )
    ));

    // array<str> args ()
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "args",
            LanFunction::ParamStruct{ },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
				std::vector<shared_ptr<LanVariable>> argVars;
                for (auto& arg : CommandLineInterface::Args)
                    argVars.push_back(make_shared<LanVariable>(LanType(LanTypeEnum::TypeString), arg));
				return std::make_shared<LanVariable>(
					LanType(LanTypeEnum::TypeArray, { LanType(LanTypeEnum::TypeString) }),
					LanVariable::LanValue{ argVars }
				);
            }
        )
    ));

    // nil dump ()
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "dump",
            LanFunction::ParamStruct{ },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                mi.Memory.dump();
                return std::nullopt;
            }
        )
    ));

    // nil dumpCache ()
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "dumpCache",
            LanFunction::ParamStruct{ },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                for (auto& [code, content] : mi.BlockMap) {
                    std::cout << std::format("{}[{}]: {}", code, content.length(), content) << std::endl;
                }
                return std::nullopt;
            }
        )
    ));

    // nil dumpType (type T)
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "dumpType",
            LanFunction::ParamStruct{ { "T", LanTypeEnum::TypeType }},
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                LanType t = std::get<LanType>(args[0]->Value);
                uint32_t value = static_cast<uint32_t>(t.BaseType);
                std::cout << std::bitset<32>(value) << std::endl;
                std::cout << LanType::BaseTypeToString(t.BaseType) << std::endl;
                if (t.Archetype.has_value())
                for (auto& ar : t.Archetype.value()) {
                    std::cout << '\t' << ar.ToString() << std::endl;
                }
                return std::nullopt;
            }
        )
    ));

    // nil dumpClass (class: cls)
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "dumpClass",
            LanFunction::ParamStruct{ { "cls", LanType(LanTypeEnum::TypeClass) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                shared_ptr<LanClass> cls = std::get<shared_ptr<LanClass>>(args[0]->Value);
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
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "dumpClass",
            LanFunction::ParamStruct{ { "clsStr", LanType(LanTypeEnum::TypeString) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                shared_ptr<LanClass> cls = mi.ResolveType(std::get<std::string>(args[0]->Value)).CustomClass;
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