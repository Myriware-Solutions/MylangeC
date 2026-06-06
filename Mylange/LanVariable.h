// LanVariable.h
#pragma once
#include <variant>
#include <string>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

class LanIterableEngine;
class LanClass;
class LanCasting;

#include "LanType.h"


class LanVariable
{
public:
	using LanValue = std::variant<
		bool,
		int,
		char,
		std::string,
		std::vector<std::unique_ptr<LanVariable>> ,
		std::unordered_map<std::string, std::unique_ptr<LanVariable>>,
		std::unique_ptr<LanIterableEngine>,
		std::unique_ptr<LanCasting>
	>;

	LanType Type;
	LanValue Value;

	LanVariable(const LanVariable&) = delete;
	LanVariable& operator=(const LanVariable&) = delete;

	LanVariable(LanVariable&&) = default;
	LanVariable& operator=(LanVariable&&) = default;

	LanVariable()
	{
		this->Type = LanType();
		this->Value = {};
	};

	std::unique_ptr<LanVariable> Clone() const;

	LanVariable(LanType type, LanValue&& value)
		: Type(std::move(type)),
		Value(std::move(value)) { };

	std::string ToString() const;

	static bool IsCompatable(const LanType& type, const LanVariable& var);
	
	bool IsCompatable(const LanType& type) const
	{
		return LanVariable::IsCompatable(type, *this);
	}

	bool Index(int& i, unique_ptr<LanVariable>&& out)
	{
		if (this->Type.IsArrayType())
		{
			auto& it = get<std::vector<std::unique_ptr<LanVariable>>>(this->Value);
			if (i < 0 || i >= it.size()) throw std::runtime_error("Array index out of bounds.");
			out = it[i]->Clone();
		}
		else throw std::runtime_error("Cannot index non-array type.");
		return false;
	}

	bool Index(const std::string& key, unique_ptr<LanVariable>&& out)
	{
		if (this->Type.IsSetType())
		{
			auto& it = get<std::unordered_map<std::string, std::unique_ptr<LanVariable>>>(this->Value);
			if (it.find(key) == it.end()) throw std::runtime_error("Key not found in set.");
			out = it[key]->Clone();
		}
		else throw std::runtime_error("Cannot index non-set type.");
		return false;
	}

	LanVariable operator+(const LanVariable& other) const;
	LanVariable operator-(const LanVariable& other) const;
	LanVariable operator*(const LanVariable& other) const;
	LanVariable operator/(const LanVariable& other) const;
	LanVariable operator==(const LanVariable& other) const;
	LanVariable operator!=(const LanVariable& other) const;
	LanVariable operator<(const LanVariable& other) const;
	LanVariable operator<=(const LanVariable& other) const;
	LanVariable operator>(const LanVariable& other) const;
	LanVariable operator>=(const LanVariable& other) const;
	bool operator&&(const LanVariable& other) const;
	bool operator||(const LanVariable& other) const;
};