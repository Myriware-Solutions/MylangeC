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
			case LanTypeEnum::TypeNil:
				return "nil";
			case LanTypeEnum::TypeBool:
				return get<bool>(this->Value) ? "true" : "false";
			case LanTypeEnum::TypeInt:
				return to_string(get<int>(this->Value));
			case LanTypeEnum::TypeChar:
				return string(1, get<char>(this->Value));
			case LanTypeEnum::TypeString:
				return get<string>(this->Value);
			case LanTypeEnum::TypeArray:
			{
				string result = "[";
				const auto& arr = get<vector<LanVariable>>(this->Value);
				for (size_t i = 0; i < arr.size(); ++i) {
					result += arr[i].ToString();
					if (i < arr.size() - 1)
						result += ", ";
				}
				result += "]";
				return result;
			}
			default:
				return "<unrepresentable value>";
		}
	}

	static bool IsCompatable(LanType& type, LanVariable var) {
		// The types exactly match
		if (type == var.Type) return true;
		// Any, or any<allowedTypes...>
		else if ((LanTypeEnum::TypeAny & type.BaseType) == LanTypeEnum::TypeAny) {
			// Truly any value
			if (popcount(static_cast<uint32_t>(type.BaseType)) == 1) return true;
			// Check to see if allowed
			if (type.ContainsArchetype(var.Type)) return true;
			return false;
		}
		// Arrays
		else if ((LanTypeEnum::TypeArray & type.BaseType & var.Type.BaseType) == LanTypeEnum::TypeArray) {
			for (auto& element : get<vector<LanVariable>>(var.Value)) {
				if (!type.ContainsArchetype(element.Type)) throw runtime_error("Element type not allowed: " + element.Type.ToString() + " in " + type.ToString());
			}
			return true;
		}
		// Sets

		// Else
		return false;
	}

	bool IsCompatable(LanType& type) {
		return LanVariable::IsCompatable(type, *this);
	}

	LanVariable operator+(const LanVariable& other) const
	{
		if (this->Type.BaseType == LanTypeEnum::TypeInt &&
			other.Type.BaseType == LanTypeEnum::TypeInt)
		{
			return LanVariable(
				LanType(LanTypeEnum::TypeInt),
				LanValue{ get<int>(this->Value) + get<int>(other.Value) }
			);
		}
		else if (this->Type.BaseType == LanTypeEnum::TypeString &&
			other.Type.BaseType == LanTypeEnum::TypeString)
		{
			return LanVariable(
				LanType(LanTypeEnum::TypeString),
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
		if (this->Type.BaseType == LanTypeEnum::TypeInt &&
			other.Type.BaseType == LanTypeEnum::TypeInt)
		{
			return LanVariable(
				LanType(LanTypeEnum::TypeInt),
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
		if (this->Type.BaseType == LanTypeEnum::TypeInt &&
			other.Type.BaseType == LanTypeEnum::TypeInt)
		{
			return LanVariable(
				LanType(LanTypeEnum::TypeInt),
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
		if (this->Type.BaseType == LanTypeEnum::TypeInt &&
			other.Type.BaseType == LanTypeEnum::TypeInt)
		{
			if (get<int>(other.Value) == 0)
			{
				throw runtime_error("Division by zero.");
			}
			return LanVariable(
				LanType(LanTypeEnum::TypeInt),
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
				LanType(LanTypeEnum::TypeBool),
				LanValue{ false }
			);
		}
		switch (this->Type.BaseType) {
			case LanTypeEnum::TypeBool:
				return LanVariable(
					LanType(LanTypeEnum::TypeBool),
					LanValue{ get<bool>(this->Value) == get<bool>(other.Value) }
				);
			case LanTypeEnum::TypeInt:
				return LanVariable(
					LanType(LanTypeEnum::TypeBool),
					LanValue{ get<int>(this->Value) == get<int>(other.Value) }
				);
			case LanTypeEnum::TypeChar:
				return LanVariable(
					LanType(LanTypeEnum::TypeBool),
					LanValue{ get<char>(this->Value) == get<char>(other.Value) }
				);
			case LanTypeEnum::TypeString:
				return LanVariable(
					LanType(LanTypeEnum::TypeBool),
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
			LanType(LanTypeEnum::TypeBool),
			LanValue{ !get<bool>(eqResult.Value) }
		);
	}

	LanVariable operator<(const LanVariable& other) const
	{
		if (this->Type.BaseType == LanTypeEnum::TypeInt &&
			other.Type.BaseType == LanTypeEnum::TypeInt)
		{
			return LanVariable(
				LanType(LanTypeEnum::TypeBool),
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
		if (this->Type.BaseType == LanTypeEnum::TypeInt &&
			other.Type.BaseType == LanTypeEnum::TypeInt)
		{
			return LanVariable(
				LanType(LanTypeEnum::TypeBool),
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
		if (this->Type.BaseType == LanTypeEnum::TypeInt &&
			other.Type.BaseType == LanTypeEnum::TypeInt)
		{
			return LanVariable(
				LanType(LanTypeEnum::TypeBool),
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
		if (this->Type.BaseType == LanTypeEnum::TypeInt &&
			other.Type.BaseType == LanTypeEnum::TypeInt)
		{
			return LanVariable(
				LanType(LanTypeEnum::TypeBool),
				LanValue{ get<int>(this->Value) >= get<int>(other.Value) }
			);
		}
		else
		{
			throw runtime_error("Unsupported types for greater-than-or-equal comparison: "
				+ this->Type.ToString() + " >= " + other.Type.ToString());
		}
	}

	LanType Type;
	LanValue Value;
};

