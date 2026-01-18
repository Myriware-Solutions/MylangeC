#include "LanVariable.h"
#include "LanType.h"
#include "Utils.h"
#include "CommandLineInterface.h"
#include "MylangeInterpreter.h"

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
