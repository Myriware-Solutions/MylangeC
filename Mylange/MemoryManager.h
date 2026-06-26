// Scope.h
#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>
#include <vector>
#include <iostream>
#include "LanVariable.h"
#include "CommandLineInterface.h"

class Scope {
public:
    std::string id;                                              // e.g. "global.myFunc.ifBlock"
    Scope* parent = nullptr;
    std::unordered_map<std::string, std::shared_ptr<Scope>> children;
    std::unordered_map<std::string, LanVariable> symbols;

    explicit Scope(std::string id, Scope* parent = nullptr)
        : id(std::move(id)), parent(parent) {
    }

    // -- Symbol management --

    void define(const std::string& name, LanVariable value) {
        // prevent overriding values
        if (symbols.contains(name)) { throw runtime_error("Cannot rewrite data: " + name); }
        CommandLineInterface::DebugPrint("Define: name '" + name+ "' type '" + value.Type.ToString() + "' as value '" + value.ToString() + "'", CommandLineInterface::DebugColor::Cyan);
        symbols[name] = std::move(value);
    }

    // Walks up the tree to reassign an existing binding
    bool assign(const std::string& name, LanVariable value) {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            it->second = std::move(value);
            return true;
        }
        return parent ? parent->assign(name, std::move(value)) : false;
    }

    // Walks up the tree to find a symbol
    LanVariable* resolve(const std::string& name) {
        auto it = symbols.find(name);
        if (it != symbols.end()) return &it->second;
        return parent ? parent->resolve(name) : nullptr;
    }

    // Shorthand: last segment of the dot-separated id
    std::string localName() const {
        auto pos = id.rfind('.');
        return pos == std::string::npos ? id : id.substr(pos + 1);
    }

};


class ScopeManager {
    std::unique_ptr<Scope> root;     // owns the entire tree
    Scope* current = nullptr;        // raw pointer — never owns

public:
    ScopeManager() {
        root = std::make_unique<Scope>("global");
        current = root.get();
    }

    // -------------------------------------------------------
    // Navigation
    // -------------------------------------------------------

    // Push a new child scope under the current one.
    // The new scope's id is automatically: current->id + "." + name
    Scope* pushScope(const std::string& name) {
        std::string fullId = current->id + "." + name;
        auto child = std::make_unique<Scope>(fullId, current);
        Scope* ptr = child.get();
        current->children[name] = std::move(child);
        current = ptr;
        return current;
    }

    // Pop back to the parent scope, destroying the current one
    void popScope(bool keepNode = false) {
        if (!current->parent)
            throw std::runtime_error("Cannot pop the global scope.");

        Scope* parent = current->parent;
        if (!keepNode) parent->children.erase(current->localName()); // destroys the child + all its symbols
        current = parent;
    }

    // Jump directly to a scope by its full dot-separated id
    // e.g. navigateTo("global.myFunc.ifBlock")
    void navigateTo(const std::string& scopeId) {
        current = resolveScope(scopeId);
    }

    Scope* currentScope() { return current; }

    // -------------------------------------------------------
    // Symbol operations (always relative to current scope
    // unless a full scope id is provided)
    // -------------------------------------------------------

    // Define a symbol in the current scope
    void define(const std::string& name, LanVariable value) {
        current->define(name, std::move(value));
    }

    void define(LanVariable functionHolder)
    {
        string id = std::get<shared_ptr<LanFunction>>(functionHolder.Value)->GetId();
        current->define(id, std::move(functionHolder));
    }

    // Define a symbol in a specific scope by id
    // e.g. defineIn("global.myFunc", "x", someValue)
    void defineIn(const std::string& scopeId,
        const std::string& name,
        LanVariable value) {
        resolveScope(scopeId)->define(name, std::move(value));
    }

    void defineIn(const std::string& scopeId,
        LanVariable functionHolder)
    {
		string id = std::get<shared_ptr<LanFunction>>(functionHolder.Value)->GetId();
		resolveScope(scopeId)->define(id, std::move(functionHolder));
    }

    bool resolve(const std::string& name, std::shared_ptr<LanVariable>& out) {
        auto var = current->resolve(name);
        if (var) {
            out = std::make_shared<LanVariable>(*var);
            return true;
        }
        return false;
    }

    // Resolve a symbol — walks up from the current scope
    LanVariable* resolve(const std::string& name) {
        return current->resolve(name);
    }

    // Resolve a symbol starting from a specific scope
    LanVariable* resolveIn(const std::string& scopeId,
        const std::string& name) {
        return resolveScope(scopeId)->resolve(name);
    }

    // Reassign an existing symbol (walks up from current)
    bool assign(const std::string& name, LanVariable value) {
        return current->assign(name, std::move(value));
    }

    // -------------------------------------------------------
    // Debug
    // -------------------------------------------------------
    void dump(Scope* scope = nullptr, int depth = 0) const {
        if (!scope) scope = root.get();
        std::string indent(depth * 2, ' ');
        std::cout << indent << "[" << scope->id << "]\n";
        for (auto& [name, sym] : scope->symbols)
            std::cout << indent << "  " << name << " : " << sym.Type.ToString()  << "\n";
        for (auto& [_, child] : scope->children)
            dump(child.get(), depth + 1);
    }

    Scope* FindSiblingScope(const std::string& name) {
        Scope* node = current;

        while (node != nullptr) {
            // Check the current node's own children (siblings of node's children,
            // and also direct children of current scope)
            auto it = node->children.find(name);
            if (it != node->children.end())
                return it->second.get();

            node = node->parent;
        }

        return nullptr;
    }

    LanVariable* FindInSiblingScope(const std::string& scopeName, const std::string& symbolName) {
        Scope* sibling = FindSiblingScope(scopeName);
        if (!sibling) return nullptr;
        auto it = sibling->symbols.find(symbolName);
        if (it != sibling->symbols.end()) return &it->second;
        return nullptr;
    }

private:
    // Walk the tree by splitting the dot-separated id
    Scope* resolveScope(const std::string& scopeId) {
        // Split on '.'
        std::vector<std::string> parts;
        std::string segment;
        for (char c : scopeId) {
            if (c == '.') { parts.push_back(segment); segment.clear(); }
            else segment += c;
        }
        parts.push_back(segment);

        // Walk from root
        if (parts.empty() || parts[0] != root->id)
            throw std::runtime_error("Scope id must start with 'global': " + scopeId);

        Scope* node = root.get();
        for (size_t i = 1; i < parts.size(); i++) {
            auto it = node->children.find(parts[i]);
            if (it == node->children.end())
                throw std::runtime_error("Scope not found: " + scopeId);
            node = it->second.get();
        }
        return node;
    }
};