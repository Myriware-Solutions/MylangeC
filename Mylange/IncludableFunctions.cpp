#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>

#include "IncludableFunctions.h"
#include "LanVariable.h"
#include "LanType.h"
#include "LanFunction.h"
#include "LanIterableEngine.h"
#include "MylangeInterpreter.h"

bool IncludableFunctions::GetIncludableFunction(
    const std::string& name,
    std::unique_ptr<LanFunction>& outFunc)
{
    auto it = Functions.find(name);
    if (it == Functions.end())
        return false;

    // Clone instead of moving: Functions keeps ownership
    outFunc = it->second->Clone();
    return true;
}

unordered_map<string, unique_ptr<LanFunction>> IncludableFunctions::Functions;

static bool InitIncludableFunctions()
{

//
// standard PACKAGE
//
    IncludableFunctions::Functions.emplace(
        "to_string(any)",
        std::make_unique<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "to_string",
            std::map<std::string, LanType>{
                { "any_in", LanType(LanTypeEnum::TypeAny) }
            },
            BuiltinFunction::CoreBuiltingLogic{
                [](const std::string& scopeId,
                   MylangeInterpreter& mi,
                   const std::vector<std::unique_ptr<LanVariable>>& args)
                   -> std::optional<std::unique_ptr<LanVariable>>
                {
                    return std::make_optional(
                        std::make_unique<LanVariable>(
                            LanType(LanTypeEnum::TypeString),
                            LanVariable::LanValue{ args[0]->ToString() }
                        )
                    );
                }
            }
        )
    );

    IncludableFunctions::Functions.emplace(
        "to_int(str)",
        std::make_unique<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "to_int",
            std::map<std::string, LanType>{
                { "int_in", LanType(LanTypeEnum::TypeString) }
    },
            BuiltinFunction::CoreBuiltingLogic{
                [](const std::string& scopeId,
                   MylangeInterpreter& mi,
                   const std::vector<std::unique_ptr<LanVariable>>& args)
                   -> std::optional<std::unique_ptr<LanVariable>>
                {
                    return std::make_optional(
                        std::make_unique<LanVariable>(
                            LanType(LanTypeEnum::TypeInt),
                            LanVariable::LanValue{ std::stoi(args[0]->ToString()) }
                        )
                    );
                }
            }
        )
    );

    IncludableFunctions::Functions.emplace(
        "CurrentScope",
        std::make_unique<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "CurrentScope",
            std::map<std::string, LanType>{ },
            BuiltinFunction::CoreBuiltingLogic{
                [](const std::string& scopeId,
                   MylangeInterpreter& mi,
                   const std::vector<std::unique_ptr<LanVariable>>& args)
                   -> std::optional<std::unique_ptr<LanVariable>>
                {
                    std::cout << "[" << scopeId << "]" << endl;
                    return nullopt;
                }
            }
        )
    );

    IncludableFunctions::Functions.emplace(
        "ClassPrintout",
        std::make_unique<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "ClassPrintout",
            std::map<std::string, LanType>{ },
            BuiltinFunction::CoreBuiltingLogic{
                [](const std::string& scopeId,
                   MylangeInterpreter& mi,
                   const std::vector<std::unique_ptr<LanVariable>>& args)
                   -> std::optional<std::unique_ptr<LanVariable>>
                {
						std::cout << "Classes in scope [" << scopeId << "]:" << endl;
						for (const auto& [className, lanClass] : mi.MemBook.Classes) {
							std::cout << "    " << className << std::endl;
                            for (const auto& [methodName, methodFunc] : lanClass->Methods) {
                                std::cout << "        " << methodName << " : " << methodFunc->GetId() << std::endl;
							}
                            for (const auto& [propName, propType] : lanClass->Properties) {
                                std::cout << "        " << propName << " : " << propType.ToString() << std::endl;
                            }
                            for (const auto& [defaultName, defaultValue] : lanClass->DefaultValues) {
                                std::cout << "        " << defaultName << " = " << defaultValue->ToString() << std::endl;
							}
                        }
						return nullopt;


                }
            }
        )
    );

    IncludableFunctions::Functions.emplace(
        "typeof(any)",
        std::make_unique<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "typeof",
            std::map<std::string, LanType>{
                { "object", LanType(LanTypeEnum::TypeAny)}
            },
            BuiltinFunction::CoreBuiltingLogic{
                [](const std::string& scopeId,
                   MylangeInterpreter& mi,
                   const std::vector<std::unique_ptr<LanVariable>>& args)
                   -> std::optional<std::unique_ptr<LanVariable>>
                {
                        return std::make_optional(
                        std::make_unique<LanVariable>(
                            LanType(LanTypeEnum::TypeString),
                            LanVariable::LanValue{ args[0]->Type.ToString() }
                        )
                    );
                }
            }
        )
    );

    IncludableFunctions::Functions.emplace(
        "VariableDump",
        std::make_unique<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "VariableDump",
            std::map<std::string, LanType>{ },
            BuiltinFunction::CoreBuiltingLogic{
                [](const std::string& scopeId,
                   MylangeInterpreter& mi,
                   const std::vector<std::unique_ptr<LanVariable>>& args)
                   -> std::optional<std::unique_ptr<LanVariable>>
                {
                    std::cout << "[c:" << scopeId << "]" << endl;
                    for (auto& y : mi.MemBook.Variables) {
                        std::cout << "    [" << y.first << "] (" << y.second.Type.ToString() << ") " << y.second.ToString() << std::endl;
                    }
                    std::cout << "[end sum]" <<  std::endl;
                    return nullopt;
                }
            }
        )
    );

// io PACKAGE

    IncludableFunctions::Functions.emplace(
        "print(any)",
        std::make_unique<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "print",
            std::map<std::string, LanType>{
                { "printString", LanType(LanTypeEnum::TypeAny) }
    },
            BuiltinFunction::CoreBuiltingLogic{
                [](const std::string& scopeId,
                   MylangeInterpreter& mi,
                   const std::vector<std::unique_ptr<LanVariable>>& args)
                   -> std::optional<std::unique_ptr<LanVariable>>
                {
                    for (const auto& arg : args) {
                        std::cout << arg->ToString() << " ";
                    }
                    std::cout << std::endl;
                    return nullopt;
                }
            }
        )
    );

    IncludableFunctions::Functions.emplace(
        "input(str)",
        std::make_unique<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "input",
            std::map<std::string, LanType>{
                { "prompt", LanType(LanTypeEnum::TypeString) }
            },
            BuiltinFunction::CoreBuiltingLogic{
                [](const std::string& scopeId,
                   MylangeInterpreter& mi,
                   const std::vector<std::unique_ptr<LanVariable>>& args)
                   -> std::optional<std::unique_ptr<LanVariable>>
                {
                    string caron = args.size() > 0 ? args[0]->ToString() : "";
                    std::cout << caron;
                    string userInput;
                    std::getline(std::cin, userInput);
                    
                    return std::make_optional(
                        std::make_unique<LanVariable>(
                            LanType(LanTypeEnum::TypeString),
                            LanVariable::LanValue{ userInput }
                        )
                    );
                }
            }
        )
    );

    return true;
}

static bool _init = InitIncludableFunctions();
