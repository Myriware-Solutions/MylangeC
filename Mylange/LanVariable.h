// LanVariable.h
#pragma once
#include "LanType.h"
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class LanIterableEngine;
class LanCasting;
class LanFunction;
class LanClass;
class MylangeInterpreter;
class LanVariable; // forward declare for collections

// -- Function types --
// Builtin: implemented in C++
using BuiltinFn = std::function<LanVariable(std::vector<LanVariable>)>;

using LanArray = std::vector<std::shared_ptr<LanVariable>>;
using LanMap = std::unordered_map<std::string, std::shared_ptr<LanVariable>>;

class LanVariable {
public:
    using LanValue = std::variant<
        std::monostate,                         // null / uninitialized
        bool,
        int,
        float,
        char,
        std::string,
        LanArray,                               // array
        LanMap,                                 // map
        std::shared_ptr<LanIterableEngine>,
        std::shared_ptr<LanCasting>,
        std::shared_ptr<LanFunction>,           // user-defined or builtin function
        std::shared_ptr<LanClass>,              // class definition
        LanType 
    >;

    LanType Type;
    LanValue Value;

    // -- Constructors --
    LanVariable() : Type(), Value(std::monostate{}) {}

    LanVariable(LanType type, LanValue value)
        : Type(std::move(type)), Value(std::move(value)) {
    }

    // -- Copy & Move --
    LanVariable(const LanVariable&) = default;
    LanVariable& operator=(const LanVariable&) = default;
    LanVariable(LanVariable&&) = default;
    LanVariable& operator=(LanVariable&&) = default;

    // -- Type checks --
    bool IsNull() const { return std::holds_alternative<std::monostate>(Value); }

    bool IsCallable() const {
        return std::holds_alternative<std::shared_ptr<LanFunction>>(Value);
    }

    bool IsBracketIndexable() const {
        return (Type.IsArrayType() || Type == LanTypeEnum::TypeString);
    }

    // -- Indexing --
    std::shared_ptr<LanVariable> Index(int i) const {
        if (Type.IsArrayType()) {
            const auto& arr = std::get<LanArray>(Value);
            if (i < 0 || i >= static_cast<int>(arr.size()))
                throw std::runtime_error("Array index out of bounds.");
            return arr[i];
        }
        else if (Type == LanTypeEnum::TypeString) {
            const auto& str = std::get<string>(Value);
            if (i < 0 || i >= static_cast<int>(str.length()))
                throw std::runtime_error("Array index out of bounds.");
            return make_shared<LanVariable>(LanVariable::Char(str[i]));
        }
        else throw std::runtime_error("Type does not support int indexing: " + Type.ToString());
    }

    std::shared_ptr<LanVariable> Index(const std::string& key) const;

    std::shared_ptr<LanVariable> DotMethod(const std::string& name, LanArray params) const;

    // -- Utilities --
    std::string ToString() const;
    static bool IsCompatible(const LanType& onType, const LanType& type);
    static bool IsCompatible(const LanType& type, const LanVariable& var);
    bool IsCompatible(const LanType& type) const {
        return LanVariable::IsCompatible(type, *this);
    }

    // -- Operators --
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

    // Helper for std::visit
    template<class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;


	// Static, quick convience functions for creating LanVariable instances of specific types
    static LanVariable Nil() {
        return LanVariable(LanType(LanTypeEnum::TypeNil), LanValue{ });
    }

    static LanVariable Bool(bool value) {
        return LanVariable(LanType(LanTypeEnum::TypeBool), LanValue{ value });
    }

    static LanVariable Int(int value) {
        return LanVariable(LanType(LanTypeEnum::TypeInt), LanValue{ value });
    }

    static LanVariable Float(float value) {
        return LanVariable(LanType(LanTypeEnum::TypeFloat), LanValue{ value });
    }

    static LanVariable Char(char value) {
        return LanVariable(LanType(LanTypeEnum::TypeChar), LanValue{ value });
    }

	static LanVariable String(std::string value) {
		return LanVariable(LanType(LanTypeEnum::TypeString), LanValue{ value });
	}

    static LanVariable Array(LanArray value) {
		LanType arrayType(LanTypeEnum::TypeArray);
		for (auto& item : value) arrayType.AddArchetype(item->Type);
        return LanVariable(arrayType, LanValue{ value });
    }

    static LanVariable Set(LanMap value) {
        return LanVariable(LanType(LanTypeEnum::TypeSet), LanValue{ value });
    }
};

// -------------------------------------------------------
// Base LanFunction
// -------------------------------------------------------
class LanFunction {
public:
    using ParamStruct = std::vector<std::pair<std::string, LanType>>;

    LanType     ReturnType;
    ParamStruct Parameters;
    std::string Name;
    std::string Logic;

    LanFunction() = default;
    virtual ~LanFunction() = default;

    LanFunction(const LanType& returnType,
        const std::string& name,
        const ParamStruct& parameters,
        const std::string& logic)
        : Parameters(parameters), ReturnType(returnType), 
        Name(name), Logic(logic) {

    }

    LanFunction(const LanFunction&) = default;
    LanFunction& operator=(const LanFunction&) = default;
    LanFunction(LanFunction&&) = default;
    LanFunction& operator=(LanFunction&&) = default;


    // -------------------------------------------------------
    // ID generation
    // -------------------------------------------------------
    std::string GetId() const {
        return LanFunction::GetId(this->Name, this->Parameters);

    }

    static std::string GetId(const std::string& name,
        const ParamStruct& parameters) {
        std::vector<LanType> types;
        for (auto& [_, type] : parameters) {
			if (type == LanTypeEnum::TypeThis) continue; // skip "this" parameter in builtin class methods
            types.push_back(type);
        }
        return GetId(name, types);
    }

    static std::string GetId(const std::string& name,
        const std::vector<LanType>& paramTypes) {
        std::string id = name + "(";
        bool first = true;
        for (auto& type : paramTypes) {
            if (!first) id += ",";
            id += type.ToString();
            first = false;
        }
        return id + ")";
    }

    static std::string GetId(const std::string& name,
        const std::vector<LanVariable>& args) {
        std::string id = name + "(";
        bool first = true;
        for (auto& arg : args) {
            if (!first) id += ",";
            id += arg.Type.ToString();
            first = false;
        }
        return id + ")";
    }

    // -------------------------------------------------------
    // Execution
    // -------------------------------------------------------
    virtual std::optional<LanVariable> Execute(MylangeInterpreter& mi,
        std::vector<LanVariable> args) = 0;

};

// -------------------------------------------------------
// ScriptFunction — user-defined function
// -------------------------------------------------------
class ScriptFunction : public LanFunction {
public:
    ScriptFunction() = default;

    ScriptFunction(const LanType& returnType,
        const std::string& name,
        const ParamStruct& parameters,
        const std::string& logic)
        : LanFunction(returnType, name, parameters, logic) {
    }

    std::optional<LanVariable> Execute(MylangeInterpreter& mi,
        std::vector<LanVariable> args) override;
};

// -------------------------------------------------------
// BuiltinFunction — C++ implemented function
// -------------------------------------------------------
using BuiltinImpl = std::function<std::optional<LanVariable>(std::vector<LanVariable>)>;

class BuiltinFunction : public LanFunction {
    BuiltinImpl impl;

public:
    BuiltinFunction() = default;

    BuiltinFunction(const LanType& returnType,
        const std::string& name,
        const ParamStruct& parameters,
        BuiltinImpl impl)
        : LanFunction(returnType, name, parameters, ""),
        impl(std::move(impl)) {
    }

    std::optional<LanVariable> Execute(MylangeInterpreter& mi,
        std::vector<LanVariable> args) override {
        (void)mi;
		CommandLineInterface::DebugPrint("Executing builtin function: " + Name + " with " + std::to_string(args.size()) + " arguments.");
        auto res = impl(std::move(args));
		if (res.has_value())
		    CommandLineInterface::DebugPrint("Builtin function " + Name + " executed." + res->ToString());
		else
			CommandLineInterface::DebugPrint("Builtin function " + Name + " executed with no return value.");

		if (res.has_value() && !LanVariable::IsCompatible(this->ReturnType, res.value().Type)) {
			throw std::runtime_error("Builtin function " + this->Name + " returned value of type "
				+ res->Type.ToString() + ", but expected "
				+ this->ReturnType.ToString() + ".");
		}

        return res;
    }
};