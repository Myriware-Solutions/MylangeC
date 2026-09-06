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
using LanArray = std::vector<std::shared_ptr<LanVariable>>;

//
// dict_types.hpp
//
// Set: fixed key population (set at construction, never changes),
//      mutable values. Only Index is really "safe" to call arbitrarily;
//      Place is restricted to keys that already exist.
//
// Table: dynamic key population, grows via open-addressing + rehash
//        when the load factor is exceeded.
//
// Both implement BracketIndexable's Index/Place. Because those are
// declared `const`, and Table's Place must be able to rehash (i.e.
// reallocate its backing storage), all backing storage in both classes
// is marked `mutable`. This makes both types "logically const, physically
// mutable" from the outside -- callers holding a `const Set&` / `const
// Table&` can still trigger writes through Place. Not thread-safe as
// written; add external synchronization if these are shared across
// threads.
//
// REQUIRES: BracketIndexable's constructor must be accessible to derived
// classes (protected or public). As given in the prompt it was private
// with no explicit access specifier, which will not compile here -- see
// chat for the one-line fix.

class ColonIndexable {
public:
    ColonIndexable() = default;

    virtual std::shared_ptr<LanVariable> Index(std::string k) const = 0;
    virtual void Place(std::string k, std::shared_ptr<LanVariable> v) = 0;

};

// ===========================================================================
// Set
// ===========================================================================
class LanSet : public ColonIndexable {
public:
    // keys: the full, fixed key population. Values are default-constructed
    // (null shared_ptr) until explicitly Place()'d.
    explicit LanSet(std::vector<std::string> keys, double max_load_factor = 0.5)
        : size_(0)
    {
        if (max_load_factor <= 0.0 || max_load_factor > 1.0) {
            throw std::invalid_argument("max_load_factor must be in (0, 1]");
        }
        size_t cap = capacity_for(keys.size(), max_load_factor);
        keys_.resize(cap);
        occupied_.assign(cap, false);
        values_.resize(cap);
        mask_ = cap - 1;
        build(keys);
    }

    // --- BracketIndexable ---------------------------------------------

    std::shared_ptr<LanVariable> Index(std::string k) const override;

    // Sets cannot grow: writing a key that wasn't present at construction
    // is a hard error. Writing an existing key just updates its value.
    void Place(std::string k, std::shared_ptr<LanVariable> v);

    // --- extras ----------------------------------------------------------

    bool contains(const std::string& k) const { return find_slot(k) != npos(); }
    size_t size() const { return size_; }
    size_t capacity() const { return keys_.size(); }

    std::string ToString() const;

private:
    static size_t npos() { return static_cast<size_t>(-1); }

    static size_t next_pow2(size_t n) {
        size_t p = 1;
        while (p < n) p <<= 1;
        return p;
    }

    static size_t capacity_for(size_t key_count, double max_load_factor) {
        if (key_count == 0) return 1;
        size_t min_slots =
            static_cast<size_t>(static_cast<double>(key_count) / max_load_factor) + 1;
        return next_pow2(min_slots);
    }

    // Runs once, at construction (not const -- no need for `mutable` here).
    void build(const std::vector<std::string>& keys) {
        for (const auto& k : keys) {
            size_t idx = hasher_(k) & mask_;
            for (;;) {
                if (!occupied_[idx]) {
                    keys_[idx] = k;
                    occupied_[idx] = true;
                    ++size_;
                    break;
                }
                if (keys_[idx] == k) break; // duplicate in input: first wins
                idx = (idx + 1) & mask_;
            }
        }
    }

    size_t find_slot(const std::string& k) const {
        size_t idx = hasher_(k) & mask_;
        for (;;) {
            if (!occupied_[idx]) return npos();
            if (keys_[idx] == k) return idx;
            idx = (idx + 1) & mask_;
        }
    }

    std::vector<std::string> keys_;                       // fixed after ctor
    mutable std::vector<std::shared_ptr<LanVariable>> values_; // Place() mutates
    std::vector<bool> occupied_;                          // fixed after ctor
    size_t mask_;
    size_t size_;
    std::hash<std::string> hasher_;
};

// ===========================================================================
// Table
// ===========================================================================
class LanTable : public ColonIndexable {
public:
    explicit LanTable(double max_load_factor = 0.5)
        : mask_(0), size_(0), max_load_factor_(max_load_factor)
    {
        size_t cap = 1;
        keys_.assign(cap, std::string());
        values_.assign(cap, nullptr);
        occupied_.assign(cap, false);
        mask_ = cap - 1;
    }

    // Optional: start with an initial key set (values default to null),
    // sized so the initial keys don't immediately trigger a grow.
    explicit LanTable(const std::vector<std::string>& initial_keys,
        double max_load_factor = 0.5)
        : mask_(0), size_(0), max_load_factor_(max_load_factor)
    {
        size_t cap = next_pow2(
            static_cast<size_t>(initial_keys.size() / max_load_factor) + 1);
        if (cap == 0) cap = 1;
        keys_.assign(cap, std::string());
        values_.assign(cap, nullptr);
        occupied_.assign(cap, false);
        mask_ = cap - 1;
        for (const auto& k : initial_keys) insert_new(k, nullptr);
    }

    // --- BracketIndexable ---------------------------------------------

    std::shared_ptr<LanVariable> Index(std::string k) const override;

    // Existing key: update value in place. New key: grow (if needed),
    // then insert. This is where Table differs fundamentally from Set.
    void Place(std::string k, std::shared_ptr<LanVariable> v) override;

    // --- extras ----------------------------------------------------------

    bool contains(const std::string& k) const { return find_slot(k) != npos(); }
    size_t size() const { return size_; }
    size_t capacity() const { return keys_.size(); }

    std::string ToString() const;

private:
    static size_t npos() { return static_cast<size_t>(-1); }

    static size_t next_pow2(size_t n) {
        size_t p = 1;
        while (p < n) p <<= 1;
        return p;
    }

    bool needs_grow() const {
        return static_cast<double>(size_ + 1) >
            max_load_factor_ * static_cast<double>(keys_.size());
    }

    // Doubles capacity and reinserts every live entry under the new mask.
    // Marked const (called from Place, which is const) -- see file header
    // note about mutable storage.
    void grow() const {
        size_t old_cap = keys_.size();
        size_t new_cap = old_cap * 2;

        std::vector<std::string> old_keys = std::move(keys_);
        std::vector<std::shared_ptr<LanVariable>> old_values = std::move(values_);
        std::vector<bool> old_occupied = std::move(occupied_);

        keys_.assign(new_cap, std::string());
        values_.assign(new_cap, nullptr);
        occupied_.assign(new_cap, false);
        mask_ = new_cap - 1;
        size_ = 0; // insert_new below will re-count

        for (size_t i = 0; i < old_cap; ++i) {
            if (old_occupied[i]) {
                insert_new(old_keys[i], std::move(old_values[i]));
            }
        }
    }

    void insert_new(const std::string& k, std::shared_ptr<LanVariable> v) const {
        size_t idx = hasher_(k) & mask_;
        for (;;) {
            if (!occupied_[idx]) {
                keys_[idx] = k;
                values_[idx] = std::move(v);
                occupied_[idx] = true;
                ++size_;
                return;
            }
            idx = (idx + 1) & mask_;
        }
    }

    size_t find_slot(const std::string& k) const {
        size_t idx = hasher_(k) & mask_;
        for (;;) {
            if (!occupied_[idx]) return npos();
            if (keys_[idx] == k) return idx;
            idx = (idx + 1) & mask_;
        }
    }

    mutable std::vector<std::string> keys_;
    mutable std::vector<std::shared_ptr<LanVariable>> values_;
    mutable std::vector<bool> occupied_;
    mutable size_t mask_;
    mutable size_t size_;
    double max_load_factor_;
    std::hash<std::string> hasher_;
};

class LanVariable : public std::enable_shared_from_this<LanVariable> {
public:
    using LanValue = std::variant<
        std::monostate,                         // null / uninitialized
        bool,
        int,
        float,
        char,
        std::string,
        LanArray,                               // array
        LanSet,
        LanTable,
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
            return std::make_shared<LanVariable>(LanVariable::Char(str[i]));
        }
        else throw std::runtime_error("Type does not support int indexing: " + Type.ToString());
    }

    bool HasIndex(const std::string& key) const;
    std::shared_ptr<LanVariable> Index(const std::string& key) const;

    std::shared_ptr<LanVariable> DotMethod(MylangeInterpreter& mi, const std::string& name, LanArray params) const;

    // -- Utilities --
    std::string ToString() const;

    static bool IsAppendable(const LanType& baseType, const LanType& toAppend) {
        auto base_archetype = (baseType.BaseType & ~LanTypeEnum::TypeArray);
		if (base_archetype == LanTypeEnum::None) {
			return true; // base type is just an array, can append anything
		}
		else {
			if ((toAppend.BaseType & base_archetype) == toAppend.BaseType)
				return true; // toAppend is compatible with base archetype
			if (baseType.Archetype.has_value()) {
				for (const auto& archetype : baseType.Archetype.value()) {
					if (archetype == toAppend) return true; // toAppend matches one of the archetypes
				}
			}
		}
        return false;
    }

    static bool IsCompatible(const LanType& onType, const LanType& type);
    static bool IsCompatible(const LanType& type, const std::shared_ptr<const LanVariable> var)
    {
        return LanVariable::IsCompatible(type, var->Type);
    };
    bool IsCompatible(const LanType& type) const {
        return LanVariable::IsCompatible(type, this->Type);  // no shared_from_this needed
    }

    // -- Operators --
    std::shared_ptr<LanVariable> operator+(const std::shared_ptr<LanVariable> other) const;
    std::shared_ptr<LanVariable> operator-(const std::shared_ptr<LanVariable> other) const;
    std::shared_ptr<LanVariable> operator*(const std::shared_ptr<LanVariable> other) const;
    std::shared_ptr<LanVariable> operator/(const std::shared_ptr<LanVariable> other) const;
    std::shared_ptr<LanVariable> operator==(const std::shared_ptr<LanVariable> other) const;
    std::shared_ptr<LanVariable> operator!=(const std::shared_ptr<LanVariable> other) const;
    std::shared_ptr<LanVariable> operator<(const std::shared_ptr<LanVariable> other) const;
    std::shared_ptr<LanVariable> operator<=(const std::shared_ptr<LanVariable> other) const;
    std::shared_ptr<LanVariable> operator>(const std::shared_ptr<LanVariable> other) const;
    std::shared_ptr<LanVariable> operator>=(const std::shared_ptr<LanVariable> other) const;
    bool operator&&(const std::shared_ptr<LanVariable> other) const;
    bool operator||(const std::shared_ptr<LanVariable> other) const;

    // Helper for std::visit
    template<class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;


	// Static, quick convience functions for creating LanVariable instances of specific types
    static LanVariable Nil() {
        return LanVariable(LanType(LanTypeEnum::TypeNil), LanValue{ });
    }

    static LanVariable Any() {
        return LanVariable(LanType(LanTypeEnum::TypeAny), LanValue{ });
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

    template <typename Container>
    static LanVariable MakeDict(LanTypeEnum typeEnum,
        const std::vector<std::string>& keys,
        const std::vector<std::shared_ptr<LanVariable>>& values) {
        Container c(keys);
        for (size_t i = 0; i < keys.size(); i++)
            c.Place(keys[i], values[i]);
        return LanVariable(LanType(typeEnum), LanVariable::LanValue{ std::move(c) });
    }

    static LanVariable Class(std::shared_ptr<LanClass> cls) {
        LanType cls_type(LanTypeEnum::TypeClass);
        cls_type.CustomClass = cls;
        return LanVariable(cls_type, cls);
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
        const LanArray& args) {
        std::string id = name + "(";
        bool first = true;
        for (auto& arg : args) {
            if (!first) id += ",";
            id += arg->Type.ToString();
            first = false;
        }
        return id + ")";
    }

	static bool GetParamsTypes(LanArray args, std::vector<LanType>& outTypes) {
		for (auto& arg : args) {
			if (!arg) return false;
			outTypes.push_back(arg->Type);
		}
		return true;
	}

    // -------------------------------------------------------
    // Execution
    // -------------------------------------------------------
    virtual std::optional<std::shared_ptr<LanVariable>> Execute(MylangeInterpreter& mi,
        std::vector<std::shared_ptr<LanVariable>> args) = 0;

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

    std::optional<std::shared_ptr<LanVariable>> Execute(MylangeInterpreter& mi,
        LanArray args) override;
};

// -------------------------------------------------------
// BuiltinFunction — C++ implemented function
// -------------------------------------------------------
using BuiltinImpl = std::function<std::optional<std::shared_ptr<LanVariable>>(LanArray)>;

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

    std::optional<std::shared_ptr<LanVariable>> Execute(MylangeInterpreter& mi,
        std::vector<std::shared_ptr<LanVariable>> args) override {
        (void)mi;
		CommandLineInterface::DebugPrint("Executing builtin function: " + Name + " with " + std::to_string(args.size()) + " arguments.");
        auto res = impl(args);
		if (res.has_value())
		    CommandLineInterface::DebugPrint("Builtin function " + Name + " executed." + res.value()->ToString());
		else
			CommandLineInterface::DebugPrint("Builtin function " + Name + " executed with no return value.");

		if (res.has_value() && !LanVariable::IsCompatible(this->ReturnType, res.value()->Type)) {
			throw std::runtime_error("Builtin function '" + this->Name + "' returned value of type "
				+ res.value()->Type.ToString() + ", but expected "
				+ this->ReturnType.ToString() + ".");
		}

        return res;
    }
};