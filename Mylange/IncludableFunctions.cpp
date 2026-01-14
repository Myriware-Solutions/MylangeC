#include <iostream>
#include <string>

#include "IncludableFunctions.h"

bool IncludableFunctions::GetIncludableFunction(const string& name, LanFunction& func)
{
    if (Functions.find(name) != Functions.end()) {
        func = Functions[name];
        return true;
    }
	return false;
}

unordered_map<string, BuiltinFunction> IncludableFunctions::Functions = {
    {
        "print",
        BuiltinFunction(
            LanType(LanType::BaseTypes::TypeNil),
            "print",
            { { "printString", LanType(LanType::BaseTypes::TypeString)}},
            [](const string& scopeId, MylangeInterpreter& mi, const vector<LanVariable>& args) {
				std::cout << "[print] ";
                for (const auto& arg : args) {
                    std::cout << arg.ToString() << " ";
                }
                std::cout << std::endl;
			    return nullopt;
            }
        )
    }
};

optional<LanVariable> BuiltinFunction::Execute(const string& scopeId, MylangeInterpreter& mi, const vector<LanVariable>& args)
{
	std::cout << "Executing builtin function: " << this->Name << std::endl;
    auto result = this->CoreLogic(scopeId, mi, args);
    if (result.has_value()) {
        return result.value();
    }
    return nullopt;
}
