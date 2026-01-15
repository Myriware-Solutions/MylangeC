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
// default PACKAGE
//
    IncludableFunctions::Functions.emplace(
        "to_string(any)",
        std::make_unique<BuiltinFunction>(
            LanType(LanType::BaseTypes::TypeNil),
            "to_string",
            map<string, LanType>{{ "any_in", LanType(LanType::BaseTypes::TypeAny) }},
            [](const string& scopeId, MylangeInterpreter& mi, const vector<LanVariable>& args) {
                LanVariable var = args[0];
				return LanVariable(LanType::BaseTypes::TypeString, var.ToString());
            }
        )
    );

//
// io PACKAGE
//

    IncludableFunctions::Functions.emplace(
        "print(str)",
        std::make_unique<BuiltinFunction>(
            LanType(LanType::BaseTypes::TypeNil),
            "print",
            map<string, LanType>{{ "printString", LanType(LanType::BaseTypes::TypeString) }},
            [](const string& scopeId, MylangeInterpreter& mi, const vector<LanVariable>& args) {
                for (const auto& arg : args) {
                    std::cout << arg.ToString() << " ";
                }
                std::cout << std::endl;
                return nullopt;
            }
        )
    );

    IncludableFunctions::Functions.emplace(
        "input(str)",
        std::make_unique<BuiltinFunction>(
            LanType(LanType::BaseTypes::TypeNil),
            "input",
            map<string, LanType>{{ "prompt", LanType(LanType::BaseTypes::TypeString) }},
            [](const string& scopeId, MylangeInterpreter& mi, const vector<LanVariable>& args) {
				string caron = args.size() > 0 ? args[0].ToString() : "";
                std::cout << caron;
                string userInput;
                std::getline(std::cin, userInput);
                return LanVariable(
                    LanType(LanType::BaseTypes::TypeString),
                    userInput
				);
            }
        )
    );



    return true;
}

static bool _init = InitIncludableFunctions();
