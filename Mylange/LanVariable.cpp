#include "LanVariable.h"
#include "LanIterableEngine.h"
#include "LanClass.h"

bool LanVariable::HasIndex(const std::string& key) const
{
	if (Type.IsSetType()) {
		const auto& set = std::get<LanSet>(Value);
		return set.contains(key);
	}
	else if (Type.IsTable()) {
		const auto& table = std::get<LanTable>(Value);
		return table.contains(key);
	}
	else if (Type == LanTypeEnum::TypeCasting) {
		const auto& casting = std::get<shared_ptr<LanCasting>>(Value);
		auto& mapc = casting->Properties;
		auto it = mapc.find(key);
		if (it == mapc.end()) return false;
		else return true;
	}
	else throw std::runtime_error("Cannot index non-set type: " + Type.ToString());
}

std::shared_ptr<LanVariable> LanVariable::Index(const std::string& key) const {
	if (Type.IsSetType()) {
		const auto& set = std::get<LanSet>(Value);
		return set.Index(key);
	}
	else if (Type.IsTable()) {
		const auto& table = std::get<LanTable>(Value);
		return table.Index(key);
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
	if (this->Type.IsArrayType()) {
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
	case LanTypeEnum::TypeSet:
		return std::get<LanSet>(this->Value).ToString();
	case LanTypeEnum::TypeTable:
		return std::get<LanTable>(this->Value).ToString();
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
		for (auto& [name, prop] : casting->Properties) {
			if (result.length() > 1) result += ", ";
			result += "(" + casting->ClassInfo->Properties[name].ToString() + ")" + name + "=>(" + prop->Type.ToString() + ")" + prop->ToString();
		}
		return result + "}";
	}
	case LanTypeEnum::TypeFunction:
	{
		const auto& func = get<shared_ptr<LanFunction>>(this->Value);

		std::string param_list = "";
		for (auto& [name, param] : func->Parameters) {
			if (param_list.length() > 1) param_list += ", ";
			param_list += param.ToString() + ": " + name;
		}

		std::string result = std::format("[{}]({}) {}", func->ReturnType.ToString(), param_list, func->Name);

		
		return result;
	}
	case LanTypeEnum::TypeType:
	{
		const auto& t = get<LanType>(this->Value);
		return "Type[" + t.ToString() + "]";
	}
	default:
		return std::format("<unrepresentable value: {}>", this->Type.ToString());
	}
}

bool LanVariable::IsCompatible(const LanType& onType, const LanType& type)
{
	// Exact type match
	if (onType == type)
		return true;

	// Any or any<...>
	if ((onType.BaseType & LanTypeEnum::TypeAny) == LanTypeEnum::TypeAny)
	{
		// Truly any
		if (onType.BaseType == LanTypeEnum::TypeAny) return true;
		// Could be in Archetype
		return onType.ContainsArchetype(type);
	}

	// Array types
	if (onType.IsArrayType() && type.IsArrayType())
	{
		// Check is type is empty (fulfills any array), array<Empty>
		if (((type.BaseType & ~LanTypeEnum::TypeArray) == LanTypeEnum::None) && !type.Archetype.has_value())
			return true;
		// Check if base var is an array<any>
		if (onType.BaseType == (LanTypeEnum::TypeArray | LanTypeEnum::TypeAny))
			return true;

	}

	CommandLineInterface::DebugPrint("Type compatibility check failed: " + onType.ToString() + " vs " + type.ToString(), CommandLineInterface::DebugColor::Yellow);

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

//



std::shared_ptr<LanVariable> LanSet::Index(std::string k) const {
	size_t slot = find_slot(k);
	if (slot == npos()) {
		throw std::out_of_range("Set::Index: key not present: " + k);
	}
	return values_[slot];
}

// Sets cannot grow: writing a key that wasn't present at construction
// is a hard error. Writing an existing key just updates its value.
void LanSet::Place(std::string k, std::shared_ptr<LanVariable> v) {
	size_t slot = find_slot(k);
	if (slot == npos()) {
		throw std::out_of_range(
			"Set::Place: cannot add new key to a fixed Set: " + k);
	}
	values_[slot] = std::move(v);
}


std::shared_ptr<LanVariable> LanTable::Index(std::string k) const {
	size_t slot = find_slot(k);
	if (slot == npos()) {
		throw std::out_of_range("Table::Index: key not present: " + k);
	}
	return values_[slot];
}

std::string LanSet::ToString() const {
	std::string result = "(";
	bool first = true;
	for (size_t i = 0; i < keys_.size(); ++i) {
		if (occupied_[i]) {
			if (!first) result += ", ";
			result += keys_[i] + "=> " +
				(values_[i] ? values_[i]->ToString() : "null");
			first = false;
		}
	}
	result += ")";
	return result;
}

// Existing key: update value in place. New key: grow (if needed),
// then insert. This is where Table differs fundamentally from Set.
void LanTable::Place(std::string k, std::shared_ptr<LanVariable> v) {
	size_t slot = find_slot(k);
	if (slot != npos()) {
		values_[slot] = std::move(v);
		return;
	}
	if (needs_grow()) grow();
	insert_new(k, std::move(v));
}

std::string LanTable::ToString() const {
	std::string result = "{";
	bool first = true;
	for (size_t i = 0; i < keys_.size(); ++i) {
		if (occupied_[i]) {
			if (!first) result += ", ";
			result += keys_[i] + " => " +
				(values_[i] ? values_[i]->ToString() : "null");
			first = false;
		}
	}
	result += "}";
	return result;
}