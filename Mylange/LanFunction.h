#pragma once
#include <string>
#include <vector>
#include <map>

#include "LanType.h"
#include "LanVariable.h"

class MylangeInterpreter;

using namespace std;

class LanFunction
{
public:
	LanFunction() {};
	LanFunction(const LanType& returnType, const std::string& name,
		const map<string, LanType>& parameters, const std::string& logic)
		: ReturnType(returnType), Name(name), Parameters(parameters), Logic(logic) {
	};

	string GetId() const;
	static string GetId(const string& name, const map<string, LanType>& parameters) {
		vector<LanType> valueTypes;
		for (const auto& [paramName, paramType] : parameters) {
			valueTypes.push_back(paramType);
		}
		return GetId(name, valueTypes);
	}
	static string GetId(const string& name, const vector<LanType>& parameterTypes) {
		string id = name + "(";
		bool first = true;
		for (const auto& paramType : parameterTypes) {
			id += (first ? "" : ",") + paramType.ToString();
			if (first) first = false;
		}
		id += ")";
		return id;
	}

	virtual optional<LanVariable> Execute(const string& scopeId, MylangeInterpreter& mi, const vector<LanVariable>& args);

	LanType ReturnType;
	string Name;
	map<string, LanType> Parameters;
	string Logic;
};

