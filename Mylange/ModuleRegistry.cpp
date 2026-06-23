#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include "LanVariable.h"
#include "ModuleRegistry.h"

void ModuleRegistry::RegisterHardwires(MylangeInterpreter& mi)
{
    // exit
    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "exit",
            std::map<std::string, LanType>{ },
            [](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                throw std::runtime_error("[exit]");
            }
        )
    ));

    // return
    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeAny),
            "return",
            std::map<std::string, LanType>{ { "o", LanType(LanTypeEnum::TypeAny) } },
            [](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (args.empty()) throw std::runtime_error("print requires 1 argument.");
                return args[0];
            }
        )
    ));

    // typeof (obj: any) -> str
    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "typeof",
            std::map<std::string, LanType>{ { "o", LanType(LanTypeEnum::TypeAny) } },
            [](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (args.empty()) throw std::runtime_error("typeof requires 1 argument.");
                return LanVariable(
                    LanType(LanTypeEnum::TypeString),
                    LanVariable::LanValue{ args[0].Type.ToString() }
                );
            }
        )
    ));

    // sizeof (obj: any) -> str
    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeInt),
            "sizeof",
            std::map<std::string, LanType>{ { "o", LanType(LanTypeEnum::TypeAny) } },
            [](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (args.empty()) throw std::runtime_error("sizeof requires 1 argument.");
                return LanVariable(
                    LanType(LanTypeEnum::TypeInt),
                    LanVariable::LanValue{ static_cast<int>(sizeof(args[0].Value)) }
                );
            }
        )
    ));
}
