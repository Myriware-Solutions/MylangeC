#pragma once
#include <string>

#include "LanVariable.h"

using namespace std;

class MemoryBooker
{
public:
	MemoryBooker();
	int BookVariable(const string& scopeId, const string& name, LanVariable variable);

	//represents the scope and name, maps to the variable
	//example: "global:x" or "global.0x001A2B3C:myArray"
	unordered_map<string, LanVariable> Variables;
};

