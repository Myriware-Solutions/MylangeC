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
		vector<LanVariable>,
		unordered_map<string, LanVariable>
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

	LanVariable operator+(const LanVariable& other) const
	{
		if (this->Type.BaseType == LanType::BaseTypes::TypeInt &&
			other.Type.BaseType == LanType::BaseTypes::TypeInt)
		{
			return LanVariable(
				LanType(LanType::BaseTypes::TypeInt),
				LanValue{ get<int>(this->Value) + get<int>(other.Value) }
			);
		}
		else if (this->Type.BaseType == LanType::BaseTypes::TypeString &&
			other.Type.BaseType == LanType::BaseTypes::TypeString)
		{
			return LanVariable(
				LanType(LanType::BaseTypes::TypeString),
				LanValue{ get<string>(this->Value) + get<string>(other.Value) }
			);
		}
		else
		{
			throw runtime_error("Unsupported types for addition: "
				+ this->Type.ToString() + " + " + other.Type.ToString());
		}
	}

	LanVariable operator-(const LanVariable& other) const
	{
		if (this->Type.BaseType == LanType::BaseTypes::TypeInt &&
			other.Type.BaseType == LanType::BaseTypes::TypeInt)
		{
			return LanVariable(
				LanType(LanType::BaseTypes::TypeInt),
				LanValue{ get<int>(this->Value) - get<int>(other.Value) }
			);
		}
		else
		{
			throw runtime_error("Unsupported types for subtraction: "
				+ this->Type.ToString() + " - " + other.Type.ToString());
		}
	}

	LanVariable operator*(const LanVariable& other) const
	{
		if (this->Type.BaseType == LanType::BaseTypes::TypeInt &&
			other.Type.BaseType == LanType::BaseTypes::TypeInt)
		{
			return LanVariable(
				LanType(LanType::BaseTypes::TypeInt),
				LanValue{ get<int>(this->Value) * get<int>(other.Value) }
			);
		}
		else
		{
			throw runtime_error("Unsupported types for multiplication: "
				+ this->Type.ToString() + " * " + other.Type.ToString());
		}
	}

	LanVariable operator/(const LanVariable& other) const
	{
		if (this->Type.BaseType == LanType::BaseTypes::TypeInt &&
			other.Type.BaseType == LanType::BaseTypes::TypeInt)
		{
			if (get<int>(other.Value) == 0)
			{
				throw runtime_error("Division by zero.");
			}
			return LanVariable(
				LanType(LanType::BaseTypes::TypeInt),
				LanValue{ get<int>(this->Value) / get<int>(other.Value) }
			);
		}
		else
		{
			throw runtime_error("Unsupported types for division: "
				+ this->Type.ToString() + " / " + other.Type.ToString());
		}
	}

	LanVariable operator==(const LanVariable& other) const
	{
		if (this->Type.BaseType != other.Type.BaseType)
		{
			return LanVariable(
				LanType(LanType::BaseTypes::TypeBool),
				LanValue{ false }
			);
		}
		switch (this->Type.BaseType) {
			case LanType::BaseTypes::TypeBool:
				return LanVariable(
					LanType(LanType::BaseTypes::TypeBool),
					LanValue{ get<bool>(this->Value) == get<bool>(other.Value) }
				);
			case LanType::BaseTypes::TypeInt:
				return LanVariable(
					LanType(LanType::BaseTypes::TypeBool),
					LanValue{ get<int>(this->Value) == get<int>(other.Value) }
				);
			case LanType::BaseTypes::TypeChar:
				return LanVariable(
					LanType(LanType::BaseTypes::TypeBool),
					LanValue{ get<char>(this->Value) == get<char>(other.Value) }
				);
			case LanType::BaseTypes::TypeString:
				return LanVariable(
					LanType(LanType::BaseTypes::TypeBool),
					LanValue{ get<string>(this->Value) == get<string>(other.Value) }
				);
			default:
				throw runtime_error("Unsupported types for equality check: "
					+ this->Type.ToString() + " == " + other.Type.ToString());
		}
	}

	LanVariable operator!=(const LanVariable& other) const
	{
		LanVariable eqResult = (*this) == other;
		return LanVariable(
			LanType(LanType::BaseTypes::TypeBool),
			LanValue{ !get<bool>(eqResult.Value) }
		);
	}

	LanVariable operator<(const LanVariable& other) const
	{
		if (this->Type.BaseType == LanType::BaseTypes::TypeInt &&
			other.Type.BaseType == LanType::BaseTypes::TypeInt)
		{
			return LanVariable(
				LanType(LanType::BaseTypes::TypeBool),
				LanValue{ get<int>(this->Value) < get<int>(other.Value) }
			);
		}
		else
		{
			throw runtime_error("Unsupported types for less-than comparison: "
				+ this->Type.ToString() + " < " + other.Type.ToString());
		}
	}

	LanVariable operator<=(const LanVariable& other) const
	{
		if (this->Type.BaseType == LanType::BaseTypes::TypeInt &&
			other.Type.BaseType == LanType::BaseTypes::TypeInt)
		{
			return LanVariable(
				LanType(LanType::BaseTypes::TypeBool),
				LanValue{ get<int>(this->Value) <= get<int>(other.Value) }
			);
		}
		else
		{
			throw runtime_error("Unsupported types for less-than-or-equal comparison: "
				+ this->Type.ToString() + " <= " + other.Type.ToString());
		}
	}

	LanVariable operator>(const LanVariable& other) const
	{
		if (this->Type.BaseType == LanType::BaseTypes::TypeInt &&
			other.Type.BaseType == LanType::BaseTypes::TypeInt)
		{
			return LanVariable(
				LanType(LanType::BaseTypes::TypeBool),
				LanValue{ get<int>(this->Value) > get<int>(other.Value) }
			);
		}
		else
		{
			throw runtime_error("Unsupported types for greater-than comparison: "
				+ this->Type.ToString() + " > " + other.Type.ToString());
		}
	}

	LanVariable operator>=(const LanVariable& other) const
	{
		if (this->Type.BaseType == LanType::BaseTypes::TypeInt &&
			other.Type.BaseType == LanType::BaseTypes::TypeInt)
		{
			return LanVariable(
				LanType(LanType::BaseTypes::TypeBool),
				LanValue{ get<int>(this->Value) >= get<int>(other.Value) }
			);
		}
		else
		{
			throw runtime_error("Unsupported types for greater-than-or-equal comparison: "
				+ this->Type.ToString() + " >= " + other.Type.ToString());
		}
	}

	static bool RandomTypeConversion(const string& value, LanVariable* var);
	static optional<LanVariable> RandomTypeConversion(const string& value);


	LanType Type;
	LanValue Value;
};

