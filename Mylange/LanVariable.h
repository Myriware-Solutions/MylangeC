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