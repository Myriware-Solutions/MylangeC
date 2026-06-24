#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include <unordered_set>

//class MemoryBooker;
//class MasterFunctionTree;
class MemoryManager;

#include "MemoryBooker.h"
#include "LanVariable.h"
#include "MemoryManager.h"

struct TokenItem
{
	enum TokenType { Value, ColonExtention, BracketExtention, Method };
	TokenType type;
	std::string value;
};

class MylangeInterpreter
{
public:
	MylangeInterpreter();
	optional<LanVariable> Interpret(const string& code);
	optional<LanVariable> InterpretBlock(const string& block, const bool SkipClearing = false);
	std::optional<std::vector<TokenItem>> TokenizeComplexValue(std::string& value);
	bool ParseParameter(const string& rawParamStr, std::shared_ptr<LanVariable>& var, bool assignVar = true);
	optional<LanVariable> ParseParameter(const string& rawParamStr);
	bool RandomTypeConversion(const string& value, std::shared_ptr<LanVariable>& var);
	optional<LanVariable> RandomTypeConversion(const string& value);
	optional<LanVariable> RunFunctionStack(const string& functionStackStr);
	struct FunctionParts {
		string Name;
		vector<LanVariable> Params;
		vector<LanType> ParamTypes;
	};
	//FunctionParts GetFunctionParts(const std::string& scopeId, std::string& input);
	//optional<LanVariable> RunFunctionStackOld(const string& functionStackStr);
	
	//MemoryBooker MemBook;
	//unique_ptr<MasterFunctionTree> RegisteredFunctions;
	ScopeManager Memory;
	std::unordered_set<std::string> LoadedModules;
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

