#pragma once
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <optional>
#include <variant>
#include <functional>
#include <type_traits>
#include <bitset>
#include <bit>

#include "Utils.h"
#include "CommandLineInterface.h"

using namespace std;


enum class LanTypeEnum : uint32_t
{
	None = 0,
	TypeNil = 1 << 0,
	TypeBool = 1 << 1,
	TypeInt = 1 << 2,
	TypeFloat = 1 << 3,
	TypeChar = 1 << 4,
	TypeString = 1 << 5,

	TypeAny = 1 << 6,
	TypeUnknown = 1 << 7,

	TypeArray = 1 << 8,
	TypeSet = 1 << 9,
	TypeCasting = 1 << 10,

	HasArchetype = 1 << 11
	// Unions are not their own types, rather, they are
	// represented if more than 1 bit is set.
	// Arrays and sets have the Array or Set bit set,
	// as well as the bits for their allowed types.
};

inline LanTypeEnum operator|(LanTypeEnum a, LanTypeEnum b)
{
	return static_cast<LanTypeEnum>(
		static_cast<uint32_t>(a) |
		static_cast<uint32_t>(b)
		);
}

inline LanTypeEnum operator&(LanTypeEnum a, LanTypeEnum b)
{
	return static_cast<LanTypeEnum>((static_cast<uint32_t>(a) &
		static_cast<uint32_t>(b)));
}

inline LanTypeEnum& operator&=(LanTypeEnum& a, LanTypeEnum b)
{
	a = a & b;
	return a;
}

inline LanTypeEnum& operator|=(LanTypeEnum& a, LanTypeEnum b)
{
	a = a | b;
	return a;
}

inline LanTypeEnum operator~(LanTypeEnum f)
{
	return static_cast<LanTypeEnum>(
		~static_cast<uint32_t>(f)
		);
}

//inline LanTypeEnum operator==(LanTypeEnum a, LanTypeEnum b)
//{
//	return a == b;
//}

class LanType {
protected:
	// Maps the values to their accepted string names.
	inline static const unordered_map<LanTypeEnum, vector<string>> BaseTypeMap = {
		{ LanTypeEnum::TypeNil, {"nil"}},
		{ LanTypeEnum::TypeBool, {"bool", "boolean"}},
		{ LanTypeEnum::TypeInt, {"int", "integer"}},
		{ LanTypeEnum::TypeFloat, {"float"}},
		{ LanTypeEnum::TypeChar, {"char", "character"}},
		{ LanTypeEnum::TypeString, {"str", "string"}},
		{ LanTypeEnum::TypeArray, {"arr", "array"}},
		{ LanTypeEnum::TypeSet, {"set"}},
		{ LanTypeEnum::TypeCasting, {"casting"}},
		{ LanTypeEnum::TypeUnknown, {"unknown"}},
		{ LanTypeEnum::TypeAny, {"any"}}
	};

	inline static const regex TypeMatchPattern = regex(R"((\w+)(?:\s*<(.*)>)?)");
public:

	LanTypeEnum BaseType;
	optional<vector<LanType>> Archetype;

	LanType() {
		this->BaseType = LanTypeEnum::None;
		this->Archetype = nullopt;
	}
	LanType(LanTypeEnum baseType) {
		this->BaseType = baseType;
		this->Archetype = nullopt;
	}
	LanType(LanTypeEnum baseType, vector<LanType> archetype) {
		this->BaseType = baseType;
		this->Archetype = archetype;
	}

	bool IsArrayType() const {
		return (this->BaseType & LanTypeEnum::TypeArray) == LanTypeEnum::TypeArray;
	}

	bool IsSetType() const {
		return (this->BaseType & LanTypeEnum::TypeSet) == LanTypeEnum::TypeSet;
	}

	bool IsComplex() const {
		return this->IsArrayType() || this->IsSetType() || this->Archetype.has_value();
	}

	static string BaseTypeToString(LanTypeEnum flag) {
		for (auto& pair : BaseTypeMap) {
			if (pair.first == flag) {
				return pair.second[0];
			}
		}
		return "unknown";
	}

	static string ToStringFromBits(LanTypeEnum bits) {
		string result = "";
		for (auto& y : Utils::SplitFlags(static_cast<uint32_t>(bits))) {
			CommandLineInterface::DebugPrint("Processing base type flag: " + to_string(y), 2);
			if (!result.empty()) result += "|";
			result += BaseTypeToString(static_cast<LanTypeEnum>(y));
		}
		return result;
	}

	static string ToString(const LanType& type) {
		if (type.IsArrayType()) {
			string result = "array<";
			result += ToString(type.BaseType & ~LanTypeEnum::TypeArray);
			if (type.Archetype) {
				result += "_adv";
			}
			return result + ">";
		}
		else if ((type.BaseType & LanTypeEnum::TypeAny) == LanTypeEnum::TypeAny) {
			string result = "any<";
			result += ToString(type.BaseType & ~LanTypeEnum::TypeAny);
			if (type.Archetype) {
				result += "_adv";
			}
			return result + ">";
		}
		else if (type.IsSetType()) {
			return "set";
		}
		else {
			CommandLineInterface::DebugPrint("Converting base type to string: " + to_string(static_cast<uint32_t>(type.BaseType)), 1);
			return ToStringFromBits(type.BaseType);
		}
	}

	string ToString() const {
		return LanType::ToString(*this);
	}

	bool ContainsArchetype(const LanType& type) {
		if (type.IsComplex() && this->Archetype.has_value()) {
			for (auto& archetype : *this->Archetype) {
				if (type == archetype) return true;
			}
			return false;
		}
		else if (!type.IsComplex()) {
			return (this->BaseType & type.BaseType) == type.BaseType;
		}
		else return false;
	}

	bool operator==(const LanType& other) const {
		if (this->BaseType != other.BaseType) return false;
		if (this->Archetype.has_value() != other.Archetype.has_value()) return false;
		if (this->Archetype.has_value()) {
			const auto& a1 = this->Archetype.value();
			const auto& a2 = other.Archetype.value();
			if (a1.size() != a2.size()) return false;
			for (size_t i = 0; i < a1.size(); ++i) {
				if (a1[i] != a2[i]) return false;
			}
		}
		return true;
	}

	static LanType FromString(const std::string& typeStringRaw)
	{
		CommandLineInterface::DebugPrint("Parsing type from string: " + typeStringRaw, 1);
		string typeString = Utils::TrimString(typeStringRaw);
		
		smatch match;
		regex_match(typeString, match, TypeMatchPattern);
		string base_type_str = match[1];
		LanTypeEnum base_type = LanTypeEnum::None;
		// Get the base type
		for (auto& pair : BaseTypeMap) {
			for (auto& alias : pair.second) {
				CommandLineInterface::DebugPrint("Checking: " + base_type_str + " " + alias, 2);
				if (alias == base_type_str) {
					base_type = pair.first;
					break;
				}
			}
			if (base_type != LanTypeEnum::None) break;
		}
		CommandLineInterface::DebugPrint("Base type parsed as: " + to_string(static_cast<uint32_t>(base_type)), 1);
		if (base_type == LanTypeEnum::None) throw runtime_error("Could not find primitive type: '" + base_type_str + "'");
		// Get archetype if applicable
		if (match[2].matched) {
			CommandLineInterface::DebugPrint("Doing archetype");
			vector<variant<LanTypeEnum, LanType>> archetype_vector;
			bool advanced = false;
			vector<string> archetype_strs = Utils::TopLevelSplit(match[2], '|');
			for (auto& archetype_str : archetype_strs) {
				string trimmed_str = Utils::TrimString(archetype_str);
				LanType archetype_type = LanType::FromString(trimmed_str);
				if (archetype_type.IsComplex()) {
					archetype_vector.push_back(archetype_type);
					advanced = true;
				}
				/*else if (find(archetype_vector.begin(), archetype_vector.end(), archetype_type.BaseType) != archetype_vector.end())
					throw runtime_error("Duplicate '" + trimmed_str + "' archetype type in: " + typeString);*/
				else archetype_vector.push_back(archetype_type.BaseType);
			}
			vector<LanType> result_archetype_vector;
			for (auto& archetype_value : archetype_vector) {
				visit([&](const auto& value){
					using T = decay_t<decltype(value)>;
					if constexpr (is_same_v<T, LanTypeEnum>) base_type = base_type | get<LanTypeEnum>(archetype_value);
					else if constexpr (is_same_v<T, LanType>) result_archetype_vector.push_back(get<LanType>(archetype_value));
				}, archetype_value);
			}

			if (result_archetype_vector.size() > 0)
				return LanType(base_type, result_archetype_vector);
			else return LanType(base_type);
		}
		return LanType(base_type);
	};
};