#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "LanVariable.h"
#include "LanType.h"
#include "LanFunction.h"
#include <memory>

class LanClass
{
private:
	std::unordered_map<std::string, LanType> Properties;
	std::unordered_map<std::string, std::unique_ptr<LanVariable>> DefaultValues;
	std::unordered_map<std::string, std::unique_ptr<LanFunction>> Methods;

public:
	std::string Name;


	~LanClass() = default;
	LanClass() {};

	LanClass(
		const std::string& name,
		const std::unordered_map<std::string, LanType>& properties,
		const std::unordered_map<std::string, std::unique_ptr<LanVariable>>& defaultValues,
		const std::unordered_map<std::string, std::unique_ptr<LanFunction>>& methods)
		: Name(name), Properties(properties)
	{
		// Deep copy default values
		for (const auto& [propName, propValue] : defaultValues) {
			DefaultValues[propName] = propValue->Clone();
		}
		// Deep copy methods
		for (const auto& [methodName, methodFunc] : methods) {
			Methods[methodName] = methodFunc->Clone();
		}
	};
};

