// LanVariable.h
#pragma once

#include <variant>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>

#include "LanType.h"

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
        std::shared_ptr<LanClass>               // class definition
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

    std::shared_ptr<LanVariable> Index(const std::string& key) const {
        if (!Type.IsSetType())
            throw std::runtime_error("Cannot index non-set type.");

        const auto& map = std::get<LanMap>(Value);
        auto it = map.find(key);
        if (it == map.end())
            throw std::runtime_error("Key not found in set.");

        return it->second;
    }

    std::shared_ptr<LanVariable> DotMethod(const std::string& name, LanArray params) const;

    // -- Utilities --
    std::string ToString() const;
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
        return LanVariable(LanType(LanTypeEnum::TypeArray), LanValue{ value });
    }
};

// -------------------------------------------------------
// Base LanFunction
// -------------------------------------------------------
class LanFunction {
public:
    LanType                        ReturnType;
    std::string                    Name;
    std::map<std::string, LanType> Parameters;
    std::string                    Logic;

    LanFunction() = default;
    virtual ~LanFunction() = default;

    LanFunction(const LanType& returnType,
        const std::string& name,
        const std::map<std::string, LanType>& parameters,
        const std::string& logic)
        : ReturnType(returnType), Name(name),
        Parameters(parameters), Logic(logic) {
    }

    LanFunction(const LanFunction&) = default;
    LanFunction& operator=(const LanFunction&) = default;
    LanFunction(LanFunction&&) = default;
    LanFunction& operator=(LanFunction&&) = default;

    // -------------------------------------------------------
    // ID generation
    // -------------------------------------------------------
    std::string GetId() const {
        std::vector<LanType> types;
        for (auto& [_, type] : Parameters)
            types.push_back(type);
        return GetId(Name, types);
    }

    static std::string GetId(const std::string& name,
        const std::map<std::string, LanType>& parameters) {
        std::vector<LanType> types;
        for (auto& [_, type] : parameters)
            types.push_back(type);
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
        const std::map<std::string, LanType>& parameters,
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
        const std::map<std::string, LanType>& parameters,
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
        return res;
    }
};