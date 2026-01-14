#pragma once
#include <string>
#include <unordered_map>

#include "LanVariable.h"
#include "LanType.h"
#include "LanFunction.h"

using namespace std;

class MemoryBooker
{
public:
	MemoryBooker();
	void BookVariable(const string& scopeId, const string& name, LanVariable variable);
	void RemoveVariable(const string& scopeId, const string& name);
	void BookFunction(const string& scopeId, LanFunction function);

	void ClearScope(const string& scopeId);

	//represents the scope and name, maps to the variable
	//example: "global:x" or "global.0x001A2B3C:myArray"
	unordered_map<string, LanVariable> Variables;
	unordered_map<string, LanFunction> Functions;
};

