// builtin_io.cpp
#include "LanClass.h"
#include "LanVariable.h"
#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include <iostream>

static void RegisterFile(MylangeInterpreter& mi, const std::string& scopeId) {

    // ------------ //
    //  File Class  //
    // ------------ //

	std::unordered_map<std::string, LanType> properties = {
		{ "path", LanType(LanTypeEnum::TypeString) },
		{ "mode", LanType(LanTypeEnum::TypeString) }
	};

	std::unordered_map<std::string, LanVariable> defaultValues = { };

	std::vector<std::shared_ptr<LanFunction>> methods_vector = {
		// Constructor: File
		std::make_shared<BuiltinFunction>(
			LanType(LanTypeEnum::TypeNil),
			"File",
			LanFunction::ParamStruct{
				{ "self", LanType(LanTypeEnum::TypeThis) },
				{ "path", LanType(LanTypeEnum::TypeString) },
				{ "mode", LanType(LanTypeEnum::TypeString) } },
			[](std::vector<LanVariable> args) -> std::optional<LanVariable> {
				if (args.size() < 2) throw std::runtime_error("File constructor requires 2 arguments: path and mode.");
				auto& self = std::get<std::shared_ptr<LanCasting>>(args[0].Value);
				auto& path = std::get<std::string>(args[1].Value);
				auto& mode = std::get<std::string>(args[2].Value);
				self->Properties["path"] = std::make_shared<LanVariable>(LanVariable(LanType(LanTypeEnum::TypeString), path));
				self->Properties["mode"] = std::make_shared<LanVariable>(LanVariable(LanType(LanTypeEnum::TypeString), mode));
				return nullopt;
			}
		),

		// Read method: reads the content of the file at the given path and returns it as a string.
		std::make_shared<BuiltinFunction>(
			LanType(LanTypeEnum::TypeString),
			"read",
			LanFunction::ParamStruct{
				{ "self", LanType(LanTypeEnum::TypeThis) } },
			[](std::vector<LanVariable> args) -> std::optional<LanVariable> {
				if (args.empty()) throw std::runtime_error("read requires 1 argument (self).");
				auto& self = args[0];
				auto pathVar = self.Index("path");
				if (!pathVar) throw std::runtime_error("File object has no 'path' property.");
				auto& path = std::get<std::string>(pathVar->Value);
				std::ifstream file(path);
				if (!file.is_open()) throw std::runtime_error("Failed to open file: " + path);
				std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				file.close();
				return LanVariable(LanType(LanTypeEnum::TypeString), content);
			}
		)
	};

    auto FileClass = std::make_shared<BuiltinClass>("File", properties, defaultValues, methods_vector);

	mi.Memory.defineIn(scopeId, "File", LanVariable(
		LanType(LanTypeEnum::TypeClass), FileClass
	));

}

// Register factory
static bool _registered = [] {
    ModuleRegistry::Register("file", RegisterFile);
    return true;
    }();