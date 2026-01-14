#include <iostream>
#include <unordered_map>
#include "MemoryBooker.h"
#include "CommandLineInterface.h"
#include "LanVariable.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "LanFunction.h"
#include "LanType.h"
#include "Utils.h"

MemoryBooker::MemoryBooker()
{
	CommandLineInterface::DebugPrint("MemoryBooker initialized.");
}

void MemoryBooker::BookVariable(const string& scopeId, const string& name, LanVariable variable)
{
	CommandLineInterface::DebugPrint("Booking Variable " + scopeId + ":" + name + " with " + variable.ToString());
	string fullId = scopeId + ":" + name;
	//this->Variables[fullId] = variable;
	this->Variables.emplace(fullId, variable);
}

void MemoryBooker::RemoveVariable(const string& scopeId, const string& name)
{
	string fullId = scopeId + ":" + name;
	CommandLineInterface::DebugPrint("Removing Variable " + fullId);
	this->Variables.erase(fullId);
}

bool MemoryBooker::GetVariable(const string& scopeId, const string& name, LanVariable& var)
{
	auto scopeParts = Utils::TopLevelSplit(scopeId, '.');
	for (int i = scopeParts.size() - 1; i >= 0; --i)
	{
		string currentScopeId = "";
		for (int j = 0; j <= i; ++j)
		{
			currentScopeId += (j == 0 ? "" : ".") + scopeParts[j];
		}
		if (this->GetLiteralVariable(currentScopeId, name, var))
		{
			return true;
		}
	}
	return false;
};

void MemoryBooker::BookFunction(const string& scopeId, unique_ptr<LanFunction> function)
{
	string fullId = scopeId + ":" + (function)->GetId();
	CommandLineInterface::DebugPrint("Booking Function " + fullId);
	this->Functions.emplace(fullId, std::move(function));
}

LanFunction* MemoryBooker::GetFunction(
	const string& scopeId,
	const string& name,
	const vector<LanType>& paramTypes)
{
	auto scopeParts = Utils::TopLevelSplit(scopeId, '.');
	for (int i = scopeParts.size() - 1; i >= 0; --i)
	{
		string currentScopeId = "";
		for (int j = 0; j <= i; ++j)
		{
			currentScopeId += (j == 0 ? "" : ".") + scopeParts[j];
		}
		if (LanFunction* func = this->GetLiteralFunction(currentScopeId, name, paramTypes))
		{
			return func;
		}
	}
	return nullptr;
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

bool MemoryBooker::GetLiteralVariable(const string& scopeId, const string& name, LanVariable& var)
{
	string fullId = scopeId + ":" + name;
	//CommandLineInterface::DebugPrint("Checking Variable " + fullId);
	if (this->Variables.find(fullId) != this->Variables.end())
	{
		//CommandLineInterface::DebugPrint("Variable " + fullId + " exists with value " + this->Variables[fullId].ToString());
		var = this->Variables[fullId];
		return true;
	}
	else
	{
		//CommandLineInterface::DebugPrint("Variable " + fullId + " does not exist.");
		return false;
	}
}

LanFunction* MemoryBooker::GetLiteralFunction(const string& scopeId, 
	const string& name, const vector<LanType>& paramTypes)
{
	const string fullId = scopeId + ":" + LanFunction::GetId(name, paramTypes);
	CommandLineInterface::DebugPrint("Checking Function " + fullId);

	auto it = Functions.find(fullId);
	if (it == Functions.end())
	{
		CommandLineInterface::DebugPrint("Function " + fullId + " does not exist.");
		return nullptr;
	}

	CommandLineInterface::DebugPrint("Function " + fullId + " exists.");
	return it->second.get();  // non-owning pointer
}


