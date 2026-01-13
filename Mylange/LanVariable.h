#pragma once
#include <variant>
#include <string>
#include "LanType.h"

using namespace std;

class LanVariable
{
public:
	using LanValue = variant<
		bool,
		int,
		char,
		string,
		vector<LanVariable>
	>;
	LanVariable();
	LanVariable(LanType type, LanValue value);

	string ToString() const
	{
		return this->Value.index() == 0 ? (get<bool>(this->Value) ? "true" : "false") :
			   this->Value.index() == 1 ? to_string(get<int>(this->Value)) :
			   this->Value.index() == 2 ? string(1, get<char>(this->Value)) :
			   this->Value.index() == 3 ? get<string>(this->Value) :
			   this->Value.index() == 4 ? "[Array]" :
			"Unknown";
	}

	static LanVariable RandomTypeConversion(const string& value);


	LanType Type;
	LanValue Value;
};

