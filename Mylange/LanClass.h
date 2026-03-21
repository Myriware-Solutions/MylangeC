#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "LanVariable.h"
#include "LanType.h"
#include "LanFunction.h"

/// <summary>
/// Holds the information for a class, including its name, default property values, and methods.
/// </summary>
class LanClass
{
public:
	std::string Name;
	std::unordered_map<std::string, LanType> Properties; // For inheritance
	std::unordered_map<std::string, std::unique_ptr<LanVariable>> DefaultValues;
	std::unordered_map<std::string, std::unique_ptr<LanFunction>> Methods;
	
	~LanClass() = default;
	LanClass() {};

	// Clone function for deep copying
	std::unique_ptr<LanClass> Clone() const {
		return std::make_unique<LanClass>(Name, Properties, DefaultValues, Methods);
	}

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

/// <summary>
/// Actual instance of an object, holds a reference to its class and its own property values.
/// </summary>
class LanCasting {
public:
	std::shared_ptr<LanClass> ClassInfo;
	std::unordered_map<std::string, std::unique_ptr<LanVariable>> Properties;
	LanCasting(std::shared_ptr<LanClass> classInfo)
		: ClassInfo(classInfo)
	{
		// Initialize properties with default values from the class
		for (const auto& [propName, propValue] : classInfo->DefaultValues) {
			Properties[propName] = propValue->Clone();
		}
	}
};

