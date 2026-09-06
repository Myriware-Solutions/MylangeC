// builtin_std.cpp
#include "LanVariable.h"
#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include <iostream>

static void Registerstd(MylangeInterpreter& mi, const std::string& scopeId) {

	// int to_int (str: o)
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "to_int",
            LanFunction::ParamStruct{ { "o", LanType(LanTypeEnum::TypeString) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (args.empty()) throw std::runtime_error("print requires 1 argument.");
                return std::make_shared<LanVariable>(
                    LanType(LanTypeEnum::TypeInt),
                    LanVariable::LanValue{ std::stoi(args[0]->ToString()) }
                );
            }
        )
    ));

    // str to_string (any: o)
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "to_string",
            LanFunction::ParamStruct{ { "o", LanType(LanTypeEnum::TypeAny) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (args.empty()) throw std::runtime_error("to_string requires 1 argument.");
                return std::make_shared<LanVariable>(
                    LanType(LanTypeEnum::TypeString),
                    LanVariable::LanValue{ args[0]->ToString() }
                );
            }
        )
    ));

	// bool is_compatible (type: a, type: b)
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeBool),
            "is_compatible",
            LanFunction::ParamStruct{
                { "on", LanType(LanTypeEnum::TypeType) },
                { "with", LanType(LanTypeEnum::TypeType) }
            },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
				LanType t1 = std::get<LanType>(args[0]->Value);
				LanType t2 = std::get<LanType>(args[1]->Value);


                return std::make_shared<LanVariable>(
                    LanType(LanTypeEnum::TypeBool),
                    LanVariable::LanValue{ LanVariable::IsCompatible(t1, t2) }
                );
            }
        )
    ));

    // str memof (any: o)
    mi.Memory.defineIn(scopeId, std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "memof",
            LanFunction::ParamStruct{ { "o", LanType(LanTypeEnum::TypeAny) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (args.empty()) throw std::runtime_error("to_string requires 1 argument.");
                return std::make_shared<LanVariable>(
                    LanType(LanTypeEnum::TypeString),
                    LanVariable::LanValue{ std::format("{}", static_cast<void*>(args[0].get()))}
                );
            }
        )
    ));

};

static bool _registered = [] {
    ModuleRegistry::Register("std", Registerstd);
    return true;
    }();