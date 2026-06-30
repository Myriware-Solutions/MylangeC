// ModuleRegistry.h
#pragma once
#include <unordered_map>
#include <functional>
#include <string>

class MylangeInterpreter;

// A factory is just a function that takes the interpreter and registers into a given scope
using ModuleFactory = std::function<void(MylangeInterpreter&, const std::string& scopeId)>;

class ModuleRegistry {
public:

    static void RegisterHardwires(MylangeInterpreter& mi);

    // Call this at program startup to register the factory — NOT the functions themselves
    static void Register(const std::string& moduleName, ModuleFactory factory) {
        Get()[moduleName] = std::move(factory);
    }


    // Called when `import X` is hit at runtime
    static bool Load(const std::string& moduleName, MylangeInterpreter& mi, const std::string& scopeId) {
        auto it = Get().find(moduleName);
        if (it == Get().end()) return false;
        it->second(mi, scopeId);
        return true;
    }

    static bool Has(const std::string& moduleName) {
        return Get().contains(moduleName);
    }

private:
    static std::unordered_map<std::string, ModuleFactory>& Get() {
        static std::unordered_map<std::string, ModuleFactory> registry;
        return registry;
    }
};