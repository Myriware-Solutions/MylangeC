#include <iostream>
#include <string>
#include <regex>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <utility>

#include "builtin.h"
#include "LanVariable.h"
#include "LanType.h"
#include "LanIterableEngine.h"
#include "MylangeInterpreter.h"


auto StdToString = std::make_unique<BuiltinFunction>(
    LanType(LanTypeEnum::TypeNil),
    "to_string",
    std::map<std::string, LanType>{
        { "any_in", LanType(LanTypeEnum::TypeAny) }
    },
    BuiltinFunction::CoreBuiltingLogic{
        [](const std::string& scopeId,
           MylangeInterpreter& mi,
           const LanArgs& args) -> LanReturn
        {
            return std::make_optional(
                std::make_unique<LanVariable>(
                    LanType(LanTypeEnum::TypeString),
                    LanVariable::LanValue{ args[0]->ToString() }
                )
            );
        }
    }
);

auto StdToInt = std::make_unique<BuiltinFunction>(
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
);



auto StdCurrentScope =
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
);

auto StdClassPrintout =
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
);

auto StdTypeOf =
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
);

auto StdVariableDump =
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
            std::cout << "[end sum]" << std::endl;

            mi.RegisteredFunctions->debugPrint();

            return nullopt;
        }
    }
);

void MasterFunctionRegistry::register_std(std::unique_ptr<MasterFunctionTree> tree)
{
    unique_ptr<vector<unique_ptr<BuiltinFunction>>> to_add = make_unique<vector<unique_ptr<BuiltinFunction>>>();
	//to_add->push_back(std::move(StdToString));
	//to_add->push_back(std::move(StdToInt));
 //   to_add->push_back(std::move(StdCurrentScope));
	//to_add->push_back(std::move(StdClassPrintout));
 //   to_add->push_back(std::move(StdTypeOf));
    to_add->push_back(std::move(StdVariableDump));

    tree->addFunction("std", std::move(to_add));
}