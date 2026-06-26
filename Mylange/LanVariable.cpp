#include "LanVariable.h"
#include "LanIterableEngine.h"
#include "LanClass.h"

std::shared_ptr<LanVariable> LanVariable::Index(const std::string& key) const {
	if (Type.IsSetType()) {
		const auto& map = std::get<LanMap>(Value);
		auto it = map.find(key);
		if (it == map.end())
			throw std::runtime_error("Key not found in set: " + key + " : " + this->ToString());
		return it->second;
	}
	else if (Type == LanTypeEnum::TypeCasting) {
		const auto& casting = std::get<shared_ptr<LanCasting>>(Value);
		auto& mapc = casting->Properties;
		auto it = mapc.find(key);
		if (it == mapc.end())
			throw std::runtime_error("Key not found in properties: " + key + " : " + this->ToString());
		return it->second;
	}
	else throw std::runtime_error("Cannot index non-set type: " + Type.ToString());
}

std::shared_ptr<LanVariable> LanVariable::DotMethod(const std::string& name, LanArray params) const
{
	// User class castring, not implemented
	if (this->Type == LanTypeEnum::TypeCasting) throw runtime_error("User-castring dot method, not implemented yet");
	else
	{
		// Mylange Primitive Type
		CommandLineInterface::DebugPrint("Looking for dot method '' on type '" + this->Type.ToString() + "'");
	}
	return std::shared_ptr<LanVariable>();
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
		const auto& arr = get<LanArray>(this->Value);
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
		const auto& set = get<LanMap>(this->Value);
		for (auto& pair : set) {
			if (result.length() > 1) result += ", ";
			result += pair.first + " => " + pair.second->ToString();
		}
		return result + ")";
	}
	case LanTypeEnum::TypeCasting:
	{
		string result = "{";
		const auto& casting = get<shared_ptr<LanCasting>>(this->Value);
		// Methods
		for (auto& [name, _] : casting->ClassInfo->Methods) {
			if (result.length() > 1) result += ", ";
			result += name;
		}
		// Properties
		result += "|";
		for (auto& [name, prop] : casting->Properties) {
			if (result.length() > 1) result += ", ";
			result += "(" + casting->ClassInfo->Properties[name].ToString() + ")" + name + "=>(" + prop->Type.ToString() + ")" + prop->ToString();
		}
		return result + "}";
	}
	default:
		return "<unrepresentable value>";
	}
}

bool LanVariable::IsCompatible(const LanType& type, const LanVariable& var)
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
		auto* arr = get_if<LanArray>(&var.Value);
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

//LanVariable LanVariable::DoTypeFunction(const string& scopeId, const std::string& name,
//	MylangeInterpreter& mi, const vector<unique_ptr<LanVariable>>& parameters)
//{
//	std::visit(overloaded{
//			[&](int arg) {},
//			[&](bool arg) {},
//			[&](char arg) {},
//			[&](const std::string& arg) {},
//			[&](const std::vector<std::unique_ptr<LanVariable>>& arg) {},
//			[&](const std::unordered_map<std::string, std::unique_ptr<LanVariable>>& arg) {},
//			[&](const std::unique_ptr<LanIterableEngine>& arg) {},
//			[&](const std::unique_ptr<LanCasting>& arg) {
//
//				auto v = arg->RunMethod(mi, scopeId, name, parameters);
//				return move(v);
//		
//			}
//		}, this->Value);
//	return LanVariable();
//}

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

bool LanVariable::operator&&(const LanVariable& other) const
{
	if ((this->Type.BaseType & LanTypeEnum::TypeBool) == LanTypeEnum::TypeBool &&
		(other.Type.BaseType & LanTypeEnum::TypeBool) == LanTypeEnum::TypeBool) {
		return (get<bool>(this->Value)) && (get<bool>(other.Value));
	}
	else
	{
		throw runtime_error("Unsupported types for and comparison: "
			+ this->Type.ToString() + " && " + other.Type.ToString());
	}
}

bool LanVariable::operator||(const LanVariable& other) const
{
	if ((this->Type.BaseType & LanTypeEnum::TypeBool) == LanTypeEnum::TypeBool &&
		(other.Type.BaseType & LanTypeEnum::TypeBool) == LanTypeEnum::TypeBool) {
		return (get<bool>(this->Value)) || (get<bool>(other.Value));
	}
	else
	{
		throw runtime_error("Unsupported types for or comparison: "
			+ this->Type.ToString() + " || " + other.Type.ToString());
	}
}
