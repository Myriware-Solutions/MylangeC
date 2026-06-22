#include <iostream>
#include <string>
#include <regex>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <utility>


#include "LanVariable.h"
#include "LanType.h"
#include "LanIterableEngine.h"
#include "MylangeInterpreter.h"
#include "builtin.h"

auto IoPrint = std::make_unique<BuiltinFunction>(
    LanType(LanTypeEnum::TypeNil),
    "print",
    std::map<std::string, LanType>{
        { "printString", LanType(LanTypeEnum::TypeString) }
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
);

auto IoInput = std::make_unique<BuiltinFunction>(
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
);

void MasterFunctionRegistry::register_io(MasterFunctionTree& tree)
{
    unique_ptr<vector<unique_ptr<BuiltinFunction>>> to_add = make_unique<vector<unique_ptr<BuiltinFunction>>>();
    to_add->push_back(std::move(IoPrint));
    to_add->push_back(std::move(IoInput));

    tree.addFunction("io", std::move(to_add));
}