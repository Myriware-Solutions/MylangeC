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
	optional<LanVariable> Interpret(const string& scopeId, const string& code);
	optional<LanVariable> InterpretBlock(const string& scopeId, const string& block);
	LanVariable ParseParameter(const string& scopeId, const string& paramStr);
	optional<LanVariable> RunFunctionStack(const string& scopeId, const string& functionStackStr);
	optional<LanVariable> RunFunctionStack(const string& scopeId, const string& name, const vector<LanVariable> parameters);
	MemoryBooker MemBook;
	unordered_map<string, string> BlockMap;
	size_t BlockCounter;
};

class CodeBlock
{
public:
	CodeBlock(const string& myScopeId);
	string MyScopeID;
	vector<string> lines;
	unordered_map<string, string> blockCache;
};

