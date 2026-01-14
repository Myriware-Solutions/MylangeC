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

bool MemoryBooker::GetVariable(const string& scopeId, const string& name, LanVariable& var)
{
	string fullId = scopeId + ":" + name;
	CommandLineInterface::DebugPrint("Checking Variable " + fullId);
	if (this->Variables.find(fullId) != this->Variables.end())
	{
		CommandLineInterface::DebugPrint("Variable " + fullId + " exists with value " + this->Variables[fullId].ToString());
		var = this->Variables[fullId];
		return true;
	}
	else
	{
		CommandLineInterface::DebugPrint("Variable " + fullId + " does not exist.");
		return false;
	}
};

void MemoryBooker::BookFunction(const string& scopeId, LanFunction function)
{
	string fullId = scopeId + ":" + function.GetId();
	CommandLineInterface::DebugPrint("Booking Function " + fullId);
	this->Functions[fullId] = function;
}

bool MemoryBooker::GetFunction(const string& scopeId, const string& name, vector<LanType> paramTypes, LanFunction& func)
{
	string fullId = scopeId + ":" + LanFunction::GetId(name, paramTypes);
	CommandLineInterface::DebugPrint("Checking Function " + fullId);
	if (this->Functions.find(fullId) != this->Functions.end())
	{
		CommandLineInterface::DebugPrint("Function " + fullId + " exists.");
		func = this->Functions[fullId];
		return true;
	}
	else
	{
		CommandLineInterface::DebugPrint("Function " + fullId + " does not exist.");
		return false;
	}
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


