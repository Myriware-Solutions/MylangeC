#include <regex>
#include <string>
#include <vector>
#include <optional>
#include <iostream>

#include "LanType.h"
#include "LanVariable.h"
#include "Utils.h"
#include <stdexcept>
#include "CommandLineInterface.h"

using namespace std;

const std::unordered_map<LanType::BaseTypes, std::vector<std::string>,
	std::hash<std::underlying_type_t<LanType::BaseTypes>>> LanType::BaseTypeMap = {
	{ LanType::BaseTypes::TypeNil, {"nil"}},
	{ LanType::BaseTypes::TypeBool, {"bool", "boolean"}},
	{ LanType::BaseTypes::TypeInt, {"int", "integer"}},
	{ LanType::BaseTypes::TypeFloat, {"float"}},
	{ LanType::BaseTypes::TypeChar, {"char", "character"}},
	{ LanType::BaseTypes::TypeString, {"str", "string"}},
	{ LanType::BaseTypes::TypeArray, {"arr", "array"}},
	{ LanType::BaseTypes::TypeSet, {"set"}},
	{ LanType::BaseTypes::TypeCasting, {"casting"}},
	{ LanType::BaseTypes::TypeUnknown, {"unknown"}}
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
	BaseTypes bt = this->BaseType;
	//std::cout << "BASETYPE=" << bt;

	try {
		auto it = LanType::BaseTypeMap.find(bt);
		if (it != LanType::BaseTypeMap.end() && !it->second.empty()) {
			return it->second.front();
		}
	}
	catch (const exception& e) {
		CommandLineInterface::DebugPrint("Error in ToString: " + std::string(e.what()));
		return "Unknown/";
	}
	return "Unknown/";
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


