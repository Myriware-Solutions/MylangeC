#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include <unordered_set>

class MemoryManager;

#include "LanVariable.h"
#include "MemoryManager.h"

struct TokenItem
{
	enum TokenType { Value, ColonExtention, BracketExtention, Method, PackageName, PackageMethod };
	TokenItem(TokenType t, string& v) {
		type = t;
		value = v;
	}
	TokenType type;
	std::string value;
	string type_string() const {
		switch (this->type) {
			case TokenType::Value:
				return "Value";
			case TokenType::ColonExtention:
				return "ColonExtention";
			case TokenType::BracketExtention:
				return "BracketExtention";
			case TokenType::Method:
				return "Method";
			case TokenType::PackageName:
				return "PackageName";
			case TokenType::PackageMethod:
				return "PackageMethod";
		}
	}
};

class MylangeInterpreter
{
public:
	MylangeInterpreter();
	optional<LanVariable> Interpret(const string& code);
	optional<LanVariable> InterpretBlock(const string& block, const bool SkipClearing = false);
	LanType ResolveType(const string& typeStr);
	std::vector<TokenItem> TokenizeComplexValue(std::string& value);
	bool ParseParameter(const string& rawParamStr, std::shared_ptr<LanVariable>& var, bool assignVar = true);
	LanVariable ForcedParseParameter(const string& rawParamStr);
	optional<LanVariable> ParseParameter(const string& rawParamStr);
	std::pair<std::string, std::vector<LanVariable>> GetFunctionParts(const string& functionCallStr);
	pair<shared_ptr<LanFunction>, vector<LanVariable>> FindFunction(const string& functionCallStr, std::string packagePath = "", vector<LanVariable> self = {});
	shared_ptr<LanFunction> FindFunction(const string& name, std::vector<LanType> paramTypes, const std::string path = "", bool excludeAny = false);
	void MakeParameters(string& paramString, vector<LanVariable>& paramsOut, vector<LanType>& paramTypesOut);
	bool RandomTypeConversion(const string& value, std::shared_ptr<LanVariable>& var);
	optional<LanVariable> RandomTypeConversion(const string& value);
	struct FunctionParts {
		string Name;
		vector<LanVariable> Params;
		vector<LanType> ParamTypes;
	};

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