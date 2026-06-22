#pragma once
#include <variant>
#include <string>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "LanType.h"

class LanVar {
public:
	using LanValue = std::variant<
		bool,
		int,
		float,
		char,
		std::string,
		std::vector<std::unique_ptr<LanVar>>,
		std::unordered_map<std::string, std::unique_ptr<LanVar>>
	>;

	virtual ~LanVar() = default;
	virtual std::unique_ptr<LanVar> Clone() const = 0;

	//virtual std::string ToString() const = 0;

	//virtual L_Array GetIterable() const = 0;
	//virtual LanValue Get() const = 0;

	LanTypeEnum Type = LanTypeEnum::None;
	LanValue Value;
};


class L_Nil : public LanVar
{
protected:
	LanTypeEnum Type = LanTypeEnum::TypeNil;

public:
	L_Nil() {
		this->Value = nullptr;
	}

	std::unique_ptr<LanVar> Clone() const override {
		return std::make_unique<L_Nil>();
	}
};

class L_Bool : public LanVar
{
protected:
	LanTypeEnum Type = LanTypeEnum::TypeBool;
public:
	L_Bool(bool value) {
		this->Value = value;
	}

	std::unique_ptr<LanVar> Clone() const override {
		return std::make_unique<L_Bool>(get<bool>(this->Value));
	}
};

class L_Int : public LanVar
{
protected:
	LanTypeEnum Type = LanTypeEnum::TypeInt;
public:
	L_Int(int value) {
		this->Value = value;
	}

	std::unique_ptr<LanVar> Clone() const override {
		return std::make_unique<L_Int>(get<int>(this->Value));
	}
};

class L_Float : public LanVar
{
protected:
	LanTypeEnum Type = LanTypeEnum::TypeFloat;
public:
	L_Float(float value) {
		this->Value = value;
	}

	std::unique_ptr<LanVar> Clone() const override {
		return std::make_unique<L_Float>(get<float>(this->Value));
	}
};

class L_Char : public LanVar
{
protected:
	LanTypeEnum Type = LanTypeEnum::TypeChar;
public:
	L_Char(char value) {
		this->Value = value;
	}

	std::unique_ptr<LanVar> Clone() const override {
		return std::make_unique<L_Char>(get<char>(this->Value));
	}
};

class L_String : public LanVar
{
protected:
	LanTypeEnum Type = LanTypeEnum::TypeString;
public:
	L_String(const std::string& value) {
		this->Value = value;
	}

	std::unique_ptr<LanVar> Clone() const override {
		return std::make_unique<L_String>(get<std::string>(this->Value));
	}
};

// Not implemented yet, but will be used for iterables and sets.

class L_Array : public LanVar
{
	LanTypeEnum Type = LanTypeEnum::TypeArray;
};

class L_Set : public LanVar
{
	LanTypeEnum Type = LanTypeEnum::TypeSet;
};

class L_Casting : public LanVar
{
	LanTypeEnum Type = LanTypeEnum::TypeCasting;
};

