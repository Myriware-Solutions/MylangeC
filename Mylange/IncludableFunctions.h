#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <optional>

#include "LanFunction.h"

using namespace std;

class BuiltinFunction : public LanFunction
{
public:
    using BuiltinLogic = function<optional<LanVariable>(const string& scopeId, MylangeInterpreter& mi, const vector<LanVariable>& args)>;

    BuiltinFunction() {};
    BuiltinFunction(const LanType& returnType, const std::string& name,
        const map<string, LanType>& parameters, BuiltinLogic coreLogic)
        : LanFunction(returnType, name, parameters, string("     ")), CoreLogic(coreLogic) {
    };

    optional<LanVariable> Execute(const string& scopeId, MylangeInterpreter& mi, const vector<LanVariable>& args) override;

    BuiltinLogic CoreLogic;
};


class IncludableFunctions
{
public:
    static bool GetIncludableFunction(const string& name, LanFunction& func);

    static unordered_map<string, BuiltinFunction> Functions;
};

