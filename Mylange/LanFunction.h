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
		map<string, LanType>&  parameters, const std::string& logic)
		: ReturnType(returnType), Name(name), Parameters(parameters), Logic(logic) {
	};

	string GetId();

	LanVariable Execute(const string& scopeId, MylangeInterpreter& mi, const vector<LanVariable>& args);

	LanType ReturnType;
	string Name;
	map<string, LanType> Parameters;
	string Logic;
};

