#pragma once
#include <unordered_map>
#include "MemoryBooker.h"
class MylangeInterpreter
{
public:
	MylangeInterpreter();
	int Interpret(const string& scopeId, const string& code);
	MemoryBooker MemBook;
	int Counter;
};

class CodeBlock
{
public:
	CodeBlock(const string& myScopeId);
	string MyScopeID;
};

