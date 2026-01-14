#include "LanVariable.h"
#include "LanType.h"
#include "Utils.h"
#include "CommandLineInterface.h"

LanVariable::LanVariable()
{
	this->Type = LanType();
	this->Value = {};
};

LanVariable::LanVariable(LanType type, LanValue value)
{
	this->Type = type;
	this->Value = value;
};

bool LanVariable::RandomTypeConversion(const string& value, LanVariable* var)
{
	optional<LanVariable> test = LanVariable::RandomTypeConversion(value);
	if (test.has_value())
	{
		*var = test.value();
		return true;
	}
	return false;
}

optional<LanVariable> LanVariable::RandomTypeConversion(const string& value)
{
	CommandLineInterface::DebugPrint("Attempting to convert value: " + value);
	string trimmedValue = Utils::TrimString(value);

	// nil
	if (trimmedValue == "nil")
	{
		CommandLineInterface::DebugPrint("Found nil");
		return LanVariable(
			LanType(LanType::BaseTypes::TypeNil),
			LanVariable::LanValue{});
	}
	// bool
	else if (trimmedValue == "true" || trimmedValue == "false")
	{
		CommandLineInterface::DebugPrint("Found bool");
		return LanVariable(
			LanType(LanType::BaseTypes::TypeBool),
			LanVariable::LanValue{ trimmedValue == "true" }
		);
	}
	// int
	else if (regex_match(trimmedValue, regex(R"(^-?\d+$)")))
	{
		CommandLineInterface::DebugPrint("Found int");
		return LanVariable(
			LanType(LanType::BaseTypes::TypeInt),
			LanVariable::LanValue{ stoi(trimmedValue) }
		);
	}
	//float
	else if (regex_match(trimmedValue, regex(R"(^-?\d+\.\d+$)")))
	{
		CommandLineInterface::DebugPrint("Found float");
		// Placeholder implementation
		return LanVariable(
			LanType(LanType::BaseTypes::TypeUnknown),
			LanVariable::LanValue{ }
		);
	}
	// char
	else if (regex_match(trimmedValue, regex(R"(^'.'$)")))
	{
		CommandLineInterface::DebugPrint("Found char");
		return LanVariable(
			LanType(LanType::BaseTypes::TypeChar),
			LanVariable::LanValue{ trimmedValue[1] }
		);
	}
	// string
	else if (regex_match(trimmedValue, regex(R"(^".*"$)")))
	{
		CommandLineInterface::DebugPrint("Found str");
		return LanVariable(
			LanType(LanType::BaseTypes::TypeString),
			LanVariable::LanValue{ trimmedValue.substr(1, trimmedValue.length() - 2) }
		);
	}
	// array
	else if (regex_match(trimmedValue, regex(R"(^\[(.*)\]$)")))
	{
		CommandLineInterface::DebugPrint("Found arr");
		// Placeholder implementation
		return LanVariable(
			LanType(LanType::BaseTypes::TypeUnknown),
			LanVariable::LanValue{ }
		);
	}
	// set

	// casting

	// unknown
	else return nullopt;
};
