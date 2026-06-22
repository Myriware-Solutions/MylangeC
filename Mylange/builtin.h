#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "LanFunction.h"
#include "LanVariable.h"
#include "MylangeInterpreter.h"
#include "Utils.h"
#include "CommandLineInterface.h"
#include "LanType.h"

class BuiltinFunction : public LanFunction
{
public:

	using CoreBuiltingLogic = function<optional<unique_ptr<LanVariable>>
		(const string& scopeId, MylangeInterpreter& mi, const vector<unique_ptr<LanVariable>>& args) >;

	unique_ptr<LanFunction> Clone() const override {
		return std::make_unique<BuiltinFunction>(*this);
	}

	BuiltinFunction() {};
	BuiltinFunction(const LanType& returnType, const std::string& name,
		const map<string, LanType>& parameters, CoreBuiltingLogic coreLogic)
		: LanFunction(returnType, name, parameters, ""), CoreLogic(coreLogic)
	{
	}

	std::optional<std::unique_ptr<LanVariable>> Execute(
		const std::string& scopeId,
		MylangeInterpreter& mi,
		const std::vector<std::unique_ptr<LanVariable>>& args
	) override
	{
		CommandLineInterface::DebugPrint(
			"Executing builtin function: " + this->Name
		);

		auto result = this->CoreLogic(scopeId, mi, args);

		if (result)
		{
			CommandLineInterface::DebugPrint(
				"Builtin function executed: " + result.value()->ToString()
			);
		}
		else
		{
			CommandLineInterface::DebugPrint(
				"Builtin function executed: <no result>"
			);
		}

		return result;
	}

	CoreBuiltingLogic CoreLogic;
};

struct PackageNode {
	std::unordered_map<std::string, std::unique_ptr<PackageNode>> children;
	std::unordered_map<std::string, std::unique_ptr<LanFunction>> functions;
};


class MasterFunctionTree {
public:
	MasterFunctionTree() {
		this->root = PackageNode();
	}


	PackageNode& getOrCreatePackage(const std::string& packagePath) {
		PackageNode* node = &root;

		for (const std::string& part : Utils::SplitString(packagePath, '.')) {
			auto& childPtr = node->children[part];
			if (!childPtr) {
				childPtr = std::make_unique<PackageNode>();
			}
			node = childPtr.get();
		}

		return *node;
	}

	PackageNode* findPackage(const std::string& packagePath) {
		PackageNode* node = &root;

		for (const std::string& part : Utils::SplitString(packagePath, '.')) {
			auto it = node->children.find(part);
			if (it == node->children.end()) {
				return nullptr;
			}
			node = it->second.get();
		}

		return node;
	}

	void addFunction(const std::string& packagePath,
		unique_ptr<LanFunction> function) {

		PackageNode& pkg = getOrCreatePackage(packagePath);
		pkg.functions.emplace(function->GetId(), std::move(function));
	}

	void addFunction(const std::string& packagePath,
		unique_ptr<BuiltinFunction> function) {

		PackageNode& pkg = getOrCreatePackage(packagePath);
		pkg.functions.emplace(function->GetId(), std::move(function));
	}

	void addFunction(const std::string& packagePath, std::unique_ptr<std::vector<std::unique_ptr<BuiltinFunction>>> functions)
	{
		for (auto& func : *functions) {
			addFunction(packagePath, std::move(func));
		}
	}

	LanFunction* findFunction(const std::string& packagePath, const std::string& functionId) {
		PackageNode* pkg = findPackage(packagePath);
		if (!pkg) return nullptr;
		auto it = pkg->functions.find(functionId);
		if (it == pkg->functions.end()) {
			return nullptr;
		}
		return it->second.get();
	}

	void debugPrint() const {
		debugPrintNode(root, "");
	}

	void debugPrintNode(const PackageNode& node, const std::string& path) const {
		// Print current package (skip empty root label if you want)
		std::string displayPath = path.empty() ? "<root>" : path;
		std::cout << "Package: " << displayPath << "\n";

		// Print functions in this package
		for (const auto& [funcName, function] : node.functions) {
			std::cout << "  Function: " << funcName
				<< " (overloads: " << function->GetId() << ")\n";
		}

		// Recurse into children
		for (const auto& [childName, childNode] : node.children) {
			std::string childPath = path.empty()
				? childName
				: path + "." + childName;

			debugPrintNode(*childNode, childPath);
		}
	}

private:
	PackageNode root;
};



class MasterFunctionRegistry
{
public:
	static void register_std(MasterFunctionTree& tree);
	static void register_io(MasterFunctionTree& tree);

	inline static std::unordered_map<std::string,
		std::function<void(MasterFunctionTree&)>> PackageRegistrations = {
		{ "std", [](MasterFunctionTree& tree) { MasterFunctionRegistry::register_std(tree); } },
		{ "io",  [](MasterFunctionTree& tree) { MasterFunctionRegistry::register_io(tree); } }
	};
};