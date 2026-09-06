#include "LanIterableEngine.h"
#include "LanVariable.h"
#include "MylangeInterpreter.h"
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

std::optional<std::shared_ptr<LanVariable>> ScriptFunction::Execute(MylangeInterpreter& mi, LanArray args)
{
	CommandLineInterface::DebugPrint("Executing function: " + this->Name + " with " + std::to_string(args.size()) + " arguments.");
	if (this->Parameters.size() != args.size())
	{
		throw runtime_error("Function " + this->Name + " expected "
			+ to_string(this->Parameters.size()) + " arguments, but got "
			+ to_string(args.size()) + ".");
	}

	// Create Function Runtime Scope

	mi.Memory.pushScope(this->Name + "()");

	size_t idx = 0;
	for (const auto& [paramName, paramType] : this->Parameters)
	{
		mi.Memory.define(paramName, args[idx]);
		++idx;
	}

	auto res = mi.InterpretBlock(this->Logic);

	// Destroy function runtime
	mi.Memory.popScope();

	if (res.has_value() && !res.value()->IsCompatible(this->ReturnType))
	{
		throw runtime_error("Function " + this->Name + " returned value of type "
			+ res.value()->Type.ToString() + ", but expected "
			+ this->ReturnType.ToString() + ".");
	}

	return res;
}
