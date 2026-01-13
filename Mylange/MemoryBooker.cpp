#include <iostream>
#include <unordered_map>
#include "MemoryBooker.h"
#include "CommandLineInterface.h"
#include "LanVariable.h"

MemoryBooker::MemoryBooker()
{
	CommandLineInterface::DebugPrint("MemoryBooker initialized.");
}

int MemoryBooker::BookVariable(const string& scopeId, const string& name, LanVariable variable)
{
	CommandLineInterface::DebugPrint("Booking " + scopeId + ":" + name + " with " + variable.ToString());
	string fullId = scopeId + ":" + name;
	this->Variables[fullId] = variable;
	return 0;
}
