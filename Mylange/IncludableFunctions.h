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

    using CoreBuiltingLogic = function<optional<LanVariable>(const string& scopeId, MylangeInterpreter& mi, const vector<LanVariable>& args)>;

    unique_ptr<LanFunction> Clone() const override {
        return std::make_unique<BuiltinFunction>(*this);
    }

    BuiltinFunction() {};
    BuiltinFunction(const LanType& returnType, const std::string& name,
        const map<string, LanType>& parameters, CoreBuiltingLogic coreLogic)
        : LanFunction(returnType, name, parameters, ""), CoreLogic(coreLogic)
    { }

    optional<LanVariable> Execute(const string& scopeId, MylangeInterpreter& mi,
        const vector<LanVariable>& args) override {
		CommandLineInterface::DebugPrint("Executing builtin function: " + this->Name);
        auto result = this->CoreLogic(scopeId, mi, args);
		CommandLineInterface::DebugPrint("Builtin function executed: " + result.value_or(LanVariable()).ToString());
        return result;
    };

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

