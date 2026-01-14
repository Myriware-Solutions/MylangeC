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
		switch (this->Type.BaseType) {
			case LanType::BaseTypes::TypeNil:
				return "nil";
			case LanType::BaseTypes::TypeBool:
				return get<bool>(this->Value) ? "true" : "false";
			case LanType::BaseTypes::TypeInt:
				return to_string(get<int>(this->Value));
			case LanType::BaseTypes::TypeChar:
				return string(1, get<char>(this->Value));
			case LanType::BaseTypes::TypeString:
				return get<string>(this->Value);
			default:
				return "<unrepresentable value>";
		}
	}

	static bool RandomTypeConversion(const string& value, LanVariable* var);
	static optional<LanVariable> RandomTypeConversion(const string& value);


	LanType Type;
	LanValue Value;
};

