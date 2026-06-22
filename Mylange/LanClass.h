// LanClass.h
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <memory>
#include "LanIterableEngine.h"
#include "LanType.h"
#include "LanVariable.h"
#include "LanFunction.h"

class MylangeInterpreter;

// -------------------------------------------------------
// LanClass — holds the blueprint for a class
// -------------------------------------------------------
class LanClass {
public:
    std::string                                          Name;
    std::unordered_map<std::string, LanType>             Properties;
    std::unordered_map<std::string, LanVariable>         DefaultValues;  // LanVariable is copyable now
    std::unordered_map<std::string, std::shared_ptr<LanFunction>> Methods;

    LanClass() = default;
    ~LanClass() = default;

    // Copy & move — both work, no manual Clone() needed
    LanClass(const LanClass&) = default;
    LanClass& operator=(const LanClass&) = default;
    LanClass(LanClass&&) = default;
    LanClass& operator=(LanClass&&) = default;

    LanClass(const std::string& name,
        const std::unordered_map<std::string, LanType>& properties,
        const std::unordered_map<std::string, LanVariable>& defaultValues,
        const std::unordered_map<std::string, std::shared_ptr<LanFunction>>& methods)
        : Name(name), Properties(properties),
        DefaultValues(defaultValues), Methods(methods) {
    }
};

// -------------------------------------------------------
// LanCasting — a live instance of a LanClass
// -------------------------------------------------------
class LanCasting {
public:
    std::shared_ptr<LanClass>                    ClassInfo;
    std::unordered_map<std::string, LanVariable> Properties;  // own copy per instance

    LanCasting() = default;
    ~LanCasting() = default;

    LanCasting(const LanCasting&) = default;
    LanCasting& operator=(const LanCasting&) = default;
    LanCasting(LanCasting&&) = default;
    LanCasting& operator=(LanCasting&&) = default;

    explicit LanCasting(std::shared_ptr<LanClass> classInfo)
        : ClassInfo(std::move(classInfo))
    {
        // Initialize instance properties from class defaults
        for (auto& [name, value] : ClassInfo->DefaultValues)
            Properties[name] = value;
    }

    // Run a method — scope managed by interpreter, args by value
    std::optional<LanVariable> RunMethod(MylangeInterpreter& mi,
        const std::string& methodName,
        std::vector<LanVariable> args) const {
        auto it = ClassInfo->Methods.find(methodName);
        if (it == ClassInfo->Methods.end())
            throw std::runtime_error("Method not found: " + methodName);
        return it->second->Execute(mi, std::move(args));
    }
};