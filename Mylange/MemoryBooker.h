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
	// Variables
	void BookVariable(const string& scopeId, const string& name, LanVariable variable);
	void RemoveVariable(const string& scopeId, const string& name);
	bool GetVariable(const string& scopeId, const string& name, LanVariable& var);
	// Functions
	void BookFunction(const string& scopeId, LanFunction function);
	bool GetFunction(const string& scopeId, const string& name, vector<LanType> paramTypes, LanFunction& func);

	// Clears
	void ClearScope(const string& scopeId);

	//represents the scope and name, maps to the variable
	//example: "global:x" or "global.0x001A2B3C:myArray"
	unordered_map<string, LanVariable> Variables;
	unordered_map<string, LanFunction> Functions;
};

