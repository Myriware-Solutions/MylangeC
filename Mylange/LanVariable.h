// LanVariable.h
#pragma once
#include <variant>
#include <string>

class LanIterableEngine;

#include "LanType.h"

using namespace std;

class LanVariable
{
public:
	using LanValue = variant<
		bool,
		int,
		char,
		string,
		vector<unique_ptr<LanVariable>> ,
		//unordered_map<string, unique_ptr<LanVariable>>,
		std::unique_ptr<LanIterableEngine>
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

	LanVariable(LanType type, LanValue&& value)
		: Type(std::move(type)),
		Value(std::move(value)) { };

	string ToString() const;

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
};