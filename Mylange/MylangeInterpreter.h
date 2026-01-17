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
	optional<LanVariable> ParseParameter(const string& scopeId, const string& rawParamStr);
	optional<LanVariable> RunFunctionStack(const string& scopeIdRaw, const string& functionStackStr);
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

