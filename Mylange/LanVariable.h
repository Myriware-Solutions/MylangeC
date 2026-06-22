// LanVariable.h
#pragma once
#include <variant>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include "LanType.h"
#include "LanFunction.h"

class LanIterableEngine;
class LanCasting;
class LanVariable; // forward declare for collections

// -- Function types --
// Builtin: implemented in C++
// UserFunction: defined in your language
struct UserFunction {
    std::vector<std::string> params;
    // Replace with whatever your AST node type is
    std::shared_ptr<void> body;
};

using BuiltinFn = std::function<LanVariable(std::vector<LanVariable>)>;
using LanArray = std::vector<std::shared_ptr<LanVariable>>;
using LanMap = std::unordered_map<std::string, std::shared_ptr<LanVariable>>;

class LanVariable {
public:
    using LanValue = std::variant <
        std::monostate,             // null / uninitialized
        bool,
        int,
        char,
        std::string,
        LanArray,                   // array  (was vector<unique_ptr>)
        LanMap,                     // map    (was unordered_map<unique_ptr>)
        std::shared_ptr<LanIterableEngine>,
        std::shared_ptr<LanCasting>,
        std::shared_ptr<LanFunction>,               // user-defined function
		std::shared_ptr<LanClass>,                  // class definition
        //BuiltinFn                   // builtin function
    > ;

    LanType  Type;
    LanValue Value;

    // -- Constructors --
    LanVariable() : Type(), Value(std::monostate{}) {}

    LanVariable(LanType type, LanValue value)
        : Type(std::move(type)), Value(std::move(value)) {
    }

    // -- Copy & Move (both work now) --
    LanVariable(const LanVariable&) = default;
    LanVariable& operator=(const LanVariable&) = default;
    LanVariable(LanVariable&&) = default;
    LanVariable& operator=(LanVariable&&) = default;

    // -- Type checks --
    bool IsNull()     const { return std::holds_alternative<std::monostate>(Value); }
    bool IsCallable() const {
        return std::holds_alternative<UserFunction>(Value)
            || std::holds_alternative<BuiltinFn>(Value);
    }

    // -- Indexing --
    // Returns a shared_ptr so the caller shares ownership, not a copy
    std::shared_ptr<LanVariable> Index(int i) const {
        if (!Type.IsArrayType())
            throw std::runtime_error("Cannot index non-array type.");
        auto& arr = std::get<LanArray>(Value);
        if (i < 0 || i >= static_cast<int>(arr.size()))
            throw std::runtime_error("Array index out of bounds.");
        return arr[i];
    }

    std::shared_ptr<LanVariable> Index(const std::string& key) const {
        if (!Type.IsSetType())
            throw std::runtime_error("Cannot index non-set type.");
        auto& map = std::get<LanMap>(Value);
        auto it = map.find(key);
        if (it == map.end())
            throw std::runtime_error("Key not found in set.");
        return it->second;
    }

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
};