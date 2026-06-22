// Scope.h
#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>
#include <vector>
#include "LanVariable.h"

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
    void popScope() {
        if (!current->parent)
            throw std::runtime_error("Cannot pop the global scope.");

        Scope* parent = current->parent;
        parent->children.erase(current->localName()); // destroys the child + all its symbols
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

    // Define a symbol in a specific scope by id
    // e.g. defineIn("global.myFunc", "x", someValue)
    void defineIn(const std::string& scopeId,
        const std::string& name,
        LanVariable value) {
        resolveScope(scopeId)->define(name, std::move(value));
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
        for (auto& [name, _] : scope->symbols)
            std::cout << indent << "  " << name << "\n";
        for (auto& [_, child] : scope->children)
            dump(child.get(), depth + 1);
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