#include <iostream>
#include <unordered_map>
#include "MemoryBooker.h"
#include "CommandLineInterface.h"
#include "LanVariable.h"

MemoryBooker::MemoryBooker()
{
	CommandLineInterface::DebugPrint("MemoryBooker initialized.");
}

void MemoryBooker::BookVariable(const string& scopeId, const string& name, LanVariable variable)
{
	CommandLineInterface::DebugPrint("Booking Variable " + scopeId + ":" + name + " with " + variable.ToString());
	string fullId = scopeId + ":" + name;
	this->Variables[fullId] = variable;
}

void MemoryBooker::RemoveVariable(const string& scopeId, const string& name)
{
	string fullId = scopeId + ":" + name;
	CommandLineInterface::DebugPrint("Removing Variable " + fullId);
	this->Variables.erase(fullId);
}

void MemoryBooker::BookFunction(const string& scopeId, LanFunction function)
{
	string fullId = scopeId + ":" + function.GetId();
	CommandLineInterface::DebugPrint("Booking Function " + fullId);
	this->Functions[fullId] = function;
}

void MemoryBooker::ClearScope(const string& scopeId)
{
	CommandLineInterface::DebugPrint("Clearing Scope " + scopeId);
	for (auto it = this->Variables.begin(); it != this->Variables.end(); )
	{
		if (it->first.rfind(scopeId + ":", 0) == 0)
		{
			CommandLineInterface::DebugPrint("Clearing Variable " + it->first);
			it = this->Variables.erase(it);
		}
		else
		{
			++it;
		}
	}
}


