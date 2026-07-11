// builtin_std.cpp
#include "LanVariable.h"
#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include <iostream>

static void Registerstd(MylangeInterpreter& mi, const std::string& scopeId) {

	// int to_int (str: o)
    mi.Memory.defineIn(scopeId, LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "to_int",
            LanFunction::ParamStruct{ { "o", LanType(LanTypeEnum::TypeString) } },
            [](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (args.empty()) throw std::runtime_error("print requires 1 argument.");
                return LanVariable(
                    LanType(LanTypeEnum::TypeInt),
                    LanVariable::LanValue{ std::stoi(args[0].ToString()) }
                );
            }
        )
    ));

    // str to_string (any: o)
    mi.Memory.defineIn(scopeId, LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "to_string",
            LanFunction::ParamStruct{ { "o", LanType(LanTypeEnum::TypeAny) } },
            [](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (args.empty()) throw std::runtime_error("to_string requires 1 argument.");
                return LanVariable(
                    LanType(LanTypeEnum::TypeString),
                    LanVariable::LanValue{ args[0].ToString() }
                );
            }
        )
    ));

};

static bool _registered = [] {
    ModuleRegistry::Register("std", Registerstd);
    return true;
    }();