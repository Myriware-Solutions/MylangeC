#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <optional>
#include <iostream>
#include <memory>
#include <ostream>
#include <vector>

#include "LanFunction.h"
#include "LanType.h"
#include "LanVariable.h"
#include "CommandLineInterface.h"

using namespace std;

class BuiltinFunction : public LanFunction
{
public:

    using CoreBuiltingLogic = function<optional<unique_ptr<LanVariable>>
        (const string& scopeId, MylangeInterpreter& mi, const vector<unique_ptr<LanVariable>>& args) > ;

    unique_ptr<LanFunction> Clone() const override {
        return std::make_unique<BuiltinFunction>(*this);
    }

    BuiltinFunction() {};
    BuiltinFunction(const LanType& returnType, const std::string& name,
        const map<string, LanType>& parameters, CoreBuiltingLogic coreLogic)
        : LanFunction(returnType, name, parameters, ""), CoreLogic(coreLogic)
    { }

    std::optional<std::unique_ptr<LanVariable>> Execute(
        const std::string& scopeId,
        MylangeInterpreter& mi,
        const std::vector<std::unique_ptr<LanVariable>>& args
    ) override
    {
        CommandLineInterface::DebugPrint(
            "Executing builtin function: " + this->Name
        );

        auto result = this->CoreLogic(scopeId, mi, args);

        if (result)
        {
            CommandLineInterface::DebugPrint(
                "Builtin function executed: " + result.value()->ToString()
            );
        }
        else
        {
            CommandLineInterface::DebugPrint(
                "Builtin function executed: <no result>"
            );
        }

        return result;
    }

    CoreBuiltingLogic CoreLogic;
};

    
class IncludableFunctions
{
public:
    //static bool GetIncludableFunction(const string& name, unique_ptr<BuiltinFunction>* func);
    static bool GetIncludableFunction(
        const string& name,
        unique_ptr<LanFunction>& outFunc);
    //static unordered_map<string, BuiltinFunction> Functions;
    static unordered_map<string, unique_ptr<LanFunction>> Functions;

};

