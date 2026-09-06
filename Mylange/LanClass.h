// LanClass.h
#pragma once
#include "LanIterableEngine.h"
#include "LanType.h"
#include "LanVariable.h"
#include "MylangeInterpreter.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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
    virtual ~LanClass() = default;

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

class BuiltinClass : public LanClass {
public:
    BuiltinClass(const std::string& name,
        const std::unordered_map<std::string, LanType>& properties,
        const std::unordered_map<std::string, LanVariable>& defaultValues,
        std::vector<std::shared_ptr<LanFunction>>& methods)
        {
            this->Name = name;
		    this->Properties = properties;
		    this->DefaultValues = defaultValues;
		    for (auto& method : methods) {
                this->Methods[method->GetId()] = method;
		    }
        }

};

// -------------------------------------------------------
// LanCasting — a live instance of a LanClass
// -------------------------------------------------------
class LanCasting : public std::enable_shared_from_this<LanCasting> {
public:
    std::shared_ptr<LanClass>                    ClassInfo;
    std::unordered_map<std::string, shared_ptr<LanVariable>> Properties;  // own copy per instance

    LanCasting() = default;
    ~LanCasting() = default;

    LanCasting(const LanCasting&) = default;
    LanCasting& operator=(const LanCasting&) = default;
    LanCasting(LanCasting&&) = default;
    LanCasting& operator=(LanCasting&&) = default;

    explicit LanCasting(std::shared_ptr<LanClass> classInfo)
        : ClassInfo(std::move(classInfo))
    {
        // Create nil keys for other properties
        for (auto& [name, value] : ClassInfo->Properties)
            Properties[name] = make_shared<LanVariable>(LanVariable::Nil());
        // Initialize instance properties from class defaults
        for (auto& [name, value] : ClassInfo->DefaultValues)
            Properties[name] = make_shared<LanVariable>(value);
    }

	void EditProperty(const std::string& name, const LanVariable& value) {
		auto it = Properties.find(name);
		if (it == Properties.end())
			throw std::runtime_error("Property not found: " + name);
		if (!value.IsCompatible(ClassInfo->Properties[name]))
			throw std::runtime_error("Type mismatch for property: " + name);
		it->second = make_shared<LanVariable>(value);
	}

    // Run a method — scope managed by interpreter, args by value
    std::optional<std::shared_ptr<LanVariable>> RunMethod(MylangeInterpreter& mi,
        const std::string& methodId,
        LanArray args) {

        std::optional<std::shared_ptr<LanVariable>> res;

        auto it = ClassInfo->Methods.find(methodId);
		for (auto& [name, _] : ClassInfo->Methods) {
			CommandLineInterface::DebugPrint("Available method: " + name);
		}
        if (it == ClassInfo->Methods.end())
            throw std::runtime_error("Method not found: " + methodId);
        auto& method = it->second;

		if (BuiltinClass* builtin = dynamic_cast<BuiltinClass*>(ClassInfo.get())) {
			
            auto combined_args = LanArray{ std::make_shared<LanVariable> (LanType(LanTypeEnum::TypeCasting), shared_from_this()) };
			combined_args.insert(combined_args.end(), args.begin(), args.end());
			auto y = method->Execute(mi, combined_args);

			if (y.has_value()) {
				CommandLineInterface::DebugPrint("Method returned: " + y.value()->ToString());
                return y.value();
			}
			else {
				CommandLineInterface::DebugPrint("Method returned no value.");
                return nullopt;
			}

            throw std::runtime_error("Cannot run methods on built-in classes directly.");
        }
        else {
            
            CommandLineInterface::DebugPrint("Executing method on class (" + this->ClassInfo->Name + ") : " + method->Name + " with " + std::to_string(args.size()) + " arguments.");
            if (method->Parameters.size() != args.size())
            {
                throw runtime_error("Method " + method->Name + " expected "
                    + to_string(method->Parameters.size()) + " arguments, but got "
                    + to_string(args.size()) + ".");
            }
            // Create Function Runtime Scope
            mi.Memory.pushScope(this->ClassInfo->Name + "::" + method->Name + "()");
            auto uiu = LanVariable::LanValue{ shared_from_this() };
            auto l = std::make_shared<LanVariable>(LanVariable(LanType(LanTypeEnum::TypeCasting), uiu));
            std::string this_label("this");
            mi.Memory.define(this_label, l);
            // Define parameters
            size_t idx = 0;
            for (const auto& [paramName, paramType] : method->Parameters)
            {
                mi.Memory.define(paramName, args[idx]);
                ++idx;
            }
            res = mi.InterpretBlock(method->Logic);
            // Destroy function runtime
            mi.Memory.popScope();
        }
        return res;
    }

};