#pragma once
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <optional>
#include <variant>
#include <functional>
#include <type_traits>

using namespace std;

class LanType
{
public:
	enum BaseTypes
	{
		TypeNil=0,
		TypeBool=1,
		TypeInt=2,
		TypeFloat=3,
		TypeChar=4,
		TypeString=5,
		TypeArray=6,
		TypeSet=7,
		TypeCasting=8,
		TypeUnion=9,
		TypeAny=10,
		TypeUnknown=11
	};

	static const unordered_map<BaseTypes, vector<string>,
		hash<underlying_type_t<BaseTypes>>> BaseTypeMap;
	static const regex Reg;

	LanType();
	LanType(BaseTypes baseType);
	LanType(BaseTypes baseType, LanType* archetype);

	string ToString() const;
	bool operator==(const LanType& other) const {
		return (this->BaseType == other.BaseType)
			&& (this->Archetype == other.Archetype);
		//TODO: add union checking as well
	}

	static LanType FromString(const string& typeString);

	BaseTypes BaseType;
	LanType* Archetype;
	vector<LanType> UnionTypes;

private:
	static optional<LanType::BaseTypes> GetBaseTypeFromString(const string& typeString);
	
};

