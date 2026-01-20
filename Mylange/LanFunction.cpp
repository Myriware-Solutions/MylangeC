#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>
#include <optional>

#include "LanVariable.h"
#include "LanFunction.h"
#include "MylangeInterpreter.h"
#include "LanIterableEngine.h"


string LanFunction::GetId() const {
	return LanFunction::GetId(this->Name, this->Parameters);
};

optional<unique_ptr<LanVariable>> ScriptFunction::Execute(const string& scopeId, MylangeInterpreter& mi,
	const vector<unique_ptr<LanVariable>>& args)
{
	//cout << "Function is working: " << this->Name << endl;
	string working_scope = scopeId + "." + this->GetId() + "@run";

	if (this->Parameters.size() != args.size())
	{
		throw runtime_error("Function " + this->Name + " expected " 
			+ to_string(this->Parameters.size()) + " arguments, but got " 
			+ to_string(args.size()) + ".");
	}

	size_t idx = 0;
	for (const auto& [paramName, paramType] : this->Parameters)
	{
		// Transfer ownership of the argument to the scope
		mi.MemBook.BookVariable(working_scope, paramName, std::move(const_cast<vector<unique_ptr<LanVariable>>&>(args)[idx]));
		++idx;
	}

	auto res = mi.InterpretBlock(working_scope, this->Logic);

	mi.MemBook.ClearScope(working_scope);

	if (res.has_value()) return make_optional(std::move(res.value()));
	else return make_unique<LanVariable>();
}
