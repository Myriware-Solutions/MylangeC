/*
#pragma once
#include <string>
#include <unordered_map>

#include "LanVariable.h"
#include "LanType.h"
#include "LanFunction.h"
#include "LanClass.h"

using namespace std;

class MemoryBooker
{
public:
	MemoryBooker();
	// Variables
	void BookVariable(const string& scopeId, const string& name, const unique_ptr<LanVariable> variable);
	void RebookVariable(const string& scopeId, const string& name, const unique_ptr<LanVariable> variable);
	void RemoveVariable(const string& scopeId, const string& name);
	bool GetVariable(const string& scopeId, const string& name, unique_ptr<LanVariable>& var);
	// Functions
	void BookFunction(const string& scopeId, unique_ptr<LanFunction> function);

	vector<LanFunction*> GetFunctionOverloads(
		const string& scopeId,
		const string& name);

	LanFunction* GetFunction(
		const string& scopeId,
		const string& name,
		const vector<LanType>& paramTypes);

	// Classes

	void BookClass(const string& scopeId, unique_ptr<LanClass> lanClass);
	bool GetClass(const string& scopeId, const string& name, unique_ptr<LanClass>& lanClass);

	// Clears
	void ClearScope(const string& scopeId);

	//represents the scope and name, maps to the variable
	//example: "global:x" or "global.0x001A2B3C:myArray"
	unordered_map<string, LanVariable> Variables;
	unordered_map<string, unique_ptr<LanFunction>> Functions;
	unordered_map<string, unique_ptr<LanClass>> Classes;

private:
	bool GetLiteralVariable(
		const string& scopeId,
		const string& name,
		unique_ptr<LanVariable>& var);

	LanFunction* GetLiteralFunction(
		const string& scopeId,
		const string& name,
		const vector<LanType>& paramTypes);

	bool GetClassLiteral(const string& scopeId, const string& name, unique_ptr<LanClass>& lanClass);

};

*/