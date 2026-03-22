#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "LanIterableEngine.h"
#include "LanType.h"
#include "LanVariable.h"
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
	std::unique_ptr<LanClass> Clone() const;

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

	// Function to clone this casting (deep copy)
	std::unique_ptr<LanCasting> Clone() const {
		auto clonedCasting = std::make_unique<LanCasting>(this->ClassInfo);
		// Deep copy properties
		for (const auto& [propName, propValue] : this->Properties) {
			clonedCasting->Properties[propName] = propValue->Clone();
		}
		return clonedCasting;
	}

	optional<unique_ptr<LanVariable>> RunMethod(MylangeInterpreter& mi, const string& scopeId,
		const std::string& methodName, const std::vector<std::unique_ptr<LanVariable>>& args) const;
};

