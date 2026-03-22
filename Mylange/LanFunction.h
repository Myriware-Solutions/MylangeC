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
	virtual unique_ptr<LanFunction> Clone() const = 0;
	virtual ~LanFunction() = default;
	LanFunction() {};
	LanFunction(const LanType& returnType, const std::string& name,
		const map<string, LanType>& parameters, const std::string& logic)
		: ReturnType(returnType), Name(name), Parameters(parameters), Logic(logic) {
		//cout << "Created function: " << this->GetId() << endl;
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
	static string GetId(const string& name, const vector<unique_ptr<LanVariable>>& parameters) {
		string id = name + "(";
		bool first = true;
		for (const auto& param : parameters) {
			id += (first ? "" : ",") + param->Type.ToString();
			if (first) first = false;
		}
		id += ")";
		return id;
	}

	virtual optional<unique_ptr<LanVariable>> Execute(const string& scopeId, MylangeInterpreter& mi, 
		const vector<unique_ptr<LanVariable>>& args) = 0;

	LanType ReturnType;
	string Name;
	map<string, LanType> Parameters;
	string Logic;
};

class ScriptFunction : public LanFunction
{
public:
	ScriptFunction(
		const LanType& returnType,
		const std::string& name,
		const std::map<std::string, LanType>& parameters,
		const std::string& logic)
		: LanFunction(returnType, name, parameters, logic) {
	}

	optional<unique_ptr<LanVariable>> Execute(
		const string& scopeId,
		MylangeInterpreter& mi,
		const vector<unique_ptr<LanVariable>>& args) override;

	std::unique_ptr<LanFunction> Clone() const override {
		return std::make_unique<ScriptFunction>(*this);
	}
};
