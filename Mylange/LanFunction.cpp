#include <stdexcept>
#include <vector>
#include <string>

#include "LanVariable.h"
#include "LanFunction.h"
#include "MylangeInterpreter.h"


string LanFunction::GetId() {
	string name = this->Name + "(";
	bool first = true;
	for (auto& [paramName, paramType] : Parameters) {
		name += ((first) ? "" : ",") + paramType.ToString();
		if (first) first = false;
	}
	return name + ")";
};

LanVariable LanFunction::Execute(const string& scopeId, MylangeInterpreter& mi, const vector<LanVariable>& args)
{
	string working_scope = scopeId + "." + this->GetId() + "@run";

	if (this->Parameters.size() != args.size())
	{
		throw runtime_error("Function " + this->Name + " expected " 
			+ to_string(this->Parameters.size()) + " arguments, but got " 
			+ to_string(args.size()) + ".");
	}

	for (auto& [paramName, paramType] : this->Parameters)
	{
		size_t index = &paramName - &this->Parameters.begin()->first;
		mi.MemBook.BookVariable(working_scope, paramName, args[index]);
	}

	auto res = mi.InterpretBlock(working_scope, this->Logic);

	mi.MemBook.ClearScope(working_scope);

	if (res.has_value()) return res.value();
	else return LanVariable();
}
