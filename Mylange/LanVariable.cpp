#include "LanVariable.h"
#include "LanIterableEngine.h"

std::unique_ptr<LanVariable> LanVariable::Clone() const
{
	LanValue clonedValue = std::visit(
		[](const auto& val) -> LanValue
		{
			using T = std::decay_t<decltype(val)>;

			// Simple value types (cheap copy)
			if constexpr (
				std::is_same_v<T, bool> ||
				std::is_same_v<T, int> ||
				std::is_same_v<T, char> ||
				std::is_same_v<T, std::string>
				)
			{
				return val;
			}
			// Vector of unique_ptr<LanVariable> -> deep clone
			else if constexpr (std::is_same_v<T, std::vector<std::unique_ptr<LanVariable>>>)
			{
				std::vector<std::unique_ptr<LanVariable>> result;
				result.reserve(val.size());

				for (const auto& elem : val)
				{
					if (elem)
						result.push_back(elem->Clone());
					else
						result.push_back(nullptr);
				}

				return result;
			}
			// LanIterableEngine (ignored for now)
			else if constexpr (std::is_same_v<T, std::unique_ptr<LanIterableEngine>>)
			{
				// Placeholder: deep cloning not implemented yet
				return std::unique_ptr<LanIterableEngine>{};
			}
			else if constexpr (std::is_same_v<T, std::unordered_map<std::string, std::unique_ptr<LanVariable>>>)
			{
				std::unordered_map<std::string, std::unique_ptr<LanVariable>> result;

				for (const auto& [k, v] : val)
					result.emplace(k, v ? v->Clone() : nullptr);

				return result;
			}
			else
			{
				static_assert(sizeof(T) == 0, "Unhandled LanValue type in Clone()");
			}
		},
		this->Value
	);

	return std::make_unique<LanVariable>(
		this->Type,
		std::move(clonedValue)
	);
}


string LanVariable::ToString() const
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
		const auto& arr = get<vector <unique_ptr< LanVariable >> > (this->Value);
		for (size_t i = 0; i < arr.size(); ++i) {
			result += arr[i]->ToString();
			if (i < arr.size() - 1)
				result += ", ";
		}
		result += "]";
		return result;
	}
	case LanTypeEnum::TypeSet:
	{
		string result = "(";
		const auto& set = get<unordered_map<string, unique_ptr<LanVariable>>>(this->Value);
		for (auto& pair : set) {
			if (result.length() > 1) result += ", ";
			result += pair.first + " => " + pair.second->ToString();
		}
		return result + ")";
	}
	default:
		return "<unrepresentable value>";
	}
}

bool LanVariable::IsCompatable(const LanType& type, const LanVariable& var)
{
	// Exact type match
	if (type == var.Type)
		return true;

	// Any or any<...>
	if ((type.BaseType & LanTypeEnum::TypeAny) == LanTypeEnum::TypeAny)
	{
		// Truly any
		if (std::popcount(static_cast<uint32_t>(type.BaseType)) == 1)
			return true;

		return type.ContainsArchetype(var.Type);
	}

	// Array types
	if ((type.BaseType & LanTypeEnum::TypeArray) == LanTypeEnum::TypeArray &&
		(var.Type.BaseType & LanTypeEnum::TypeArray) == LanTypeEnum::TypeArray)
	{
		auto* arr = get_if<vector<unique_ptr<LanVariable>>>(&var.Value);
		if (!arr) throw runtime_error("Uh no.");
		for (auto& element : *arr)
		{
			if (!type.ContainsArchetype(element->Type))
			{
				throw std::runtime_error(
					"Element type not allowed: " +
					element->Type.ToString() +
					" in " +
					type.ToString()
				);
			}
		}
		return true;
	}

	return false;
}

LanVariable LanVariable::operator+(const LanVariable& other) const
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

LanVariable LanVariable::operator-(const LanVariable& other) const
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

LanVariable LanVariable::operator*(const LanVariable& other) const
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

LanVariable LanVariable::operator/(const LanVariable& other) const
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

LanVariable LanVariable::operator==(const LanVariable& other) const
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

LanVariable LanVariable::operator!=(const LanVariable& other) const
{
	LanVariable eqResult = (*this) == other;
	return LanVariable(
		LanType(LanTypeEnum::TypeBool),
		LanValue{ !get<bool>(eqResult.Value) }
	);
}

LanVariable LanVariable::operator<(const LanVariable& other) const
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

LanVariable LanVariable::operator<=(const LanVariable& other) const
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

LanVariable LanVariable::operator>(const LanVariable& other) const
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

LanVariable LanVariable::operator>=(const LanVariable& other) const
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
