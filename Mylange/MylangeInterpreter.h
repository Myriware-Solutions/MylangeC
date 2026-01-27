#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>

class MemoryBooker;

#include "MemoryBooker.h"
#include "LanVariable.h"

using namespace std;

class MylangeInterpreter
{
public:
	MylangeInterpreter();
	optional<unique_ptr<LanVariable>> Interpret(const string& scopeId, const string& code);
	optional<unique_ptr<LanVariable>> InterpretBlock(const string& scopeId, const string& block, const bool SkipClearing = false);
	optional<unique_ptr<LanVariable>> ParseParameter(const string& scopeId, const string& rawParamStr);
	bool RandomTypeConversion(const string& scopeId, const string& value, unique_ptr<LanVariable>& var);
	optional<unique_ptr<LanVariable>> RandomTypeConversion(const string& scopeId, const string& value);
	optional<unique_ptr<LanVariable>> RunFunctionStack(const string& scopeIdRaw, const string& functionStackStr);
	MemoryBooker MemBook;
	vector<string> ImportedPackages;
	unordered_map<string, string> BlockMap;
	size_t BlockCounter;
	bool DebugMode = false;
};

class CodeBlock
{
public:
	CodeBlock(const string& myScopeId);
	string MyScopeID;
	vector<string> lines;
	unordered_map<string, string> blockCache;
};

