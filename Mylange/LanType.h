#pragma once
#include "CommandLineInterface.h"
#include "Utils.h"
#include <bit>
#include <bitset>
#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace std;

class LanClass;

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
	TypeTable = 1 << 10,
	TypeCasting = 1 << 11,

	TypeInterable = 1 << 12,
	TypeFunction = 1 << 13,
	TypeClass = 1 << 14,
	TypeType = 1 << 15,
	TypeThis = 1 << 16
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
		{ LanTypeEnum::TypeTable, {"table"}},
		{ LanTypeEnum::TypeCasting, {"casting"}},
		{ LanTypeEnum::TypeUnknown, {"unknown"}},
		{ LanTypeEnum::TypeAny, {"any"}},
		{ LanTypeEnum::TypeInterable, {"iterable"} },
		{ LanTypeEnum::TypeFunction, {"function", "func"} },
		{ LanTypeEnum::TypeClass, {"class"} },
		{ LanTypeEnum::TypeType, {"type"} }
	};

	inline static const regex TypeMatchPattern = regex(R"((\w+)(?:\s*<(.*)>)?)");
public:

	LanTypeEnum BaseType;
	optional<vector<LanType>> Archetype;
	shared_ptr<LanClass> CustomClass;

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

	LanType(shared_ptr<LanClass> customClass);

	void AddArchetype(const LanType& type) {
		if (type.IsComplex()) {
			if (!this->Archetype.has_value()) {
				this->Archetype = vector<LanType>{};
			}
			this->Archetype->push_back(type);
		}
		else {
			this->BaseType |= type.BaseType;
		}
	}

	bool IsArrayType() const {
		return (this->BaseType & LanTypeEnum::TypeArray) == LanTypeEnum::TypeArray;
	}

	bool IsSetType() const {
		return (this->BaseType & LanTypeEnum::TypeSet) == LanTypeEnum::TypeSet;
	}

	bool IsTable() const {
		return this->BaseType == LanTypeEnum::TypeTable;
	}

	bool IsComplex() const {
		return this->IsArrayType() || this->IsSetType() || this->Archetype.has_value();
	}

	static string BaseTypeToString(LanTypeEnum flag);
	static string ToStringFromBits(LanTypeEnum bits);
	static string ToString(const LanType& type);
	string ToString() const {
		return LanType::ToString(*this);
	}

	inline static std::string ToPackageString(const LanType& type) {
		if (type.IsArrayType()) {
			return "array";
		}
		else if (type.IsSetType()) {
			return "set";
		}
		else if ((type.BaseType & LanTypeEnum::TypeAny) == LanTypeEnum::TypeAny) {
			return "any";
		}
		else {
			return BaseTypeToString(type.BaseType);
		}
	};
	std::string ToPackageString() const {
		return LanType::ToPackageString(*this);
	}

	bool ContainsArchetype(const LanType& type) const;
	bool operator==(const LanType& other) const;
	static LanType FromString(const std::string& typeStringRaw);
	
};