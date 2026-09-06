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

struct DepthEngine
{
	std::unordered_map<char, int> depth;

	void Clear() {
		depth.clear();
	}

	bool Place(char c) {
		bool result = true;
		for (auto& b : Utils::BracketPairs) {
			if (c == b.first) {
				depth[b.first]++;
				result = true;
			}
			else if (c == b.second) {
				if (b.second == '>' && last == '=') result = false;
				else {
					depth[b.first]--;
					result = true;
				}
			}
		}
		if (!std::isspace(c)) last = c;
		return result;
	}

	void Place(std::string& str) {
		for (auto c : str) Place(c);
	}

	int Get() {
		int u = 0;
		for (auto o : depth) u += o.second;
		return u;
	}
private:
	char last = 0;

};

class MylangeInterpreter
{
public:
	MylangeInterpreter();
	std::optional<std::shared_ptr<LanVariable>> Interpret(const string& code);
	std::optional<std::shared_ptr<LanVariable>> InterpretBlock(const string& block, const bool SkipClearing = false);
	LanType ResolveType(const string& typeStr);
	std::vector<TokenItem> TokenizeComplexValue(const std::string& value);
	bool ParseParameter(const string& rawParamStr, std::shared_ptr<LanVariable>& var, bool assignVar = true);
	std::shared_ptr<LanVariable> ForcedParseParameter(const string& rawParamStr);
	std::shared_ptr<LanVariable> ResolveVariableExtentions(const string& varPattern);
	std::optional<std::shared_ptr<LanVariable>> ParseParameter(const string& rawParamStr);
	std::pair<std::string, LanArray> GetFunctionParts(const string& functionCallStr);
	pair<shared_ptr<LanFunction>, LanArray> FindFunction(const string& functionCallStr, std::string packagePath = "", LanArray self = {});
	shared_ptr<LanFunction> FindFunction(const string& name, std::vector<LanType> paramTypes, const std::string path = "", bool excludeAny = false);
	void MakeParameters(string& paramString, LanArray& paramsOut, vector<LanType>& paramTypesOut);
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