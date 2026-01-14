#include <regex>
#include <string>
#include <vector>
#include <optional>

#include "LanType.h"
#include "LanVariable.h"
#include "Utils.h"
#include <stdexcept>

using namespace std;

const unordered_map<LanType::BaseTypes, vector<string>> LanType::BaseTypeMap = {
	{ LanType::BaseTypes::TypeNil, {"nil"}},
	{ LanType::BaseTypes::TypeBool, {"bool", "boolean"}},
	{ LanType::BaseTypes::TypeInt, {"int", "integer"}},
	{ LanType::BaseTypes::TypeFloat, {"float"}},
	{ LanType::BaseTypes::TypeChar, {"char", "character"}},
	{ LanType::BaseTypes::TypeString, {"str", "string"}},
	{ LanType::BaseTypes::TypeArray, {"arr", "array"}},
	{ LanType::BaseTypes::TypeSet, {"set"}},
	{ LanType::BaseTypes::TypeCasting, {"casting"}},
};

optional<LanType::BaseTypes> LanType::GetBaseTypeFromString(const string& typeString)
{
	for (auto& pair : BaseTypeMap)
	{
		for (auto& alias : pair.second)
		{
			if (alias == typeString)
			{
				return pair.first;
			}
		}
	}
	return std::nullopt;
}

const regex LanType::Reg = regex(R"((\w+)(?:\s*<(.*)>)?)");

LanType::LanType()
{
	this->BaseType = LanType::BaseTypes::TypeUnknown;
	this->Archetype = nullptr;
}

LanType::LanType(BaseTypes baseType)
{
	this->BaseType = baseType;
	this->Archetype = nullptr;
};

LanType::LanType(BaseTypes baseType, LanType* archetype)
{
	this->BaseType = baseType;
	this->Archetype = archetype;
}

string LanType::ToString() const
{
	return LanType::BaseTypeMap.at(this->BaseType)[0];
}
;

LanType LanType::FromString(const std::string& typeString)
{
	smatch match;
	regex_search(typeString, match, LanType::Reg);
	if (match[2].matched)
	{
		auto baseTypeOpt = GetBaseTypeFromString(match[1]);
		if (baseTypeOpt.has_value())
		{
			LanType lt = LanType::FromString(match[2]);
			return LanType(baseTypeOpt.value(), &lt);
		}
		throw std::invalid_argument("Unknown type string: " + typeString);
	}
	else
	{
		auto baseTypeOpt = GetBaseTypeFromString(match[1]);
		if (baseTypeOpt.has_value())
		{
			return LanType(baseTypeOpt.value());
		}
		throw std::invalid_argument("Unknown type string: " + typeString);
	}
}
;


