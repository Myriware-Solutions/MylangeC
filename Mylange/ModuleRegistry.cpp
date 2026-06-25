#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include "LanVariable.h"
#include "ModuleRegistry.h"
#include <variant>

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

    // void debug ()
    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "debug",
            std::map<std::string, LanType>{ },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                CommandLineInterface::DebugEnabled = !CommandLineInterface::DebugEnabled;
                return std::nullopt;
            }
        )
    ));

    // void debug (bool)
    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "debug",
            std::map<std::string, LanType>{ { "o", LanType(LanTypeEnum::TypeBool) } },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (!args.empty())
                    CommandLineInterface::DebugEnabled = std::get<bool>(args[0].Value);
                return std::nullopt;
            }
        )
    ));

    // Strings
    mi.Memory.pushScope("str");
    mi.LoadedModules.insert("str");

    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "toUpper",
            std::map<std::string, LanType>{ { "o", LanType(LanTypeEnum::TypeString) } },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                string text = std::get<string>(args[0].Value);
                for (char& c : text) {
                    c = std::toupper(static_cast<unsigned char>(c));
                }
                return LanVariable::String(text);
            }
        )
    ));

    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "toLower",
            std::map<std::string, LanType>{ { "self", LanType(LanTypeEnum::TypeString) } },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                string text = std::get<string>(args[0].Value);
                for (char& c : text) {
                    c = std::tolower(static_cast<unsigned char>(c));
                }
                return LanVariable::String(text);
            }
        )
    ));

    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeChar),
            "at",
            std::map<std::string, LanType>{
                { "self", LanType(LanTypeEnum::TypeString) },
                { "index", LanType(LanTypeEnum::TypeInt) }},
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                string text = std::get<string>(args[0].Value);
                return LanVariable::Char(text[std::get<int>(args[1].Value)]);
            }
        )
    ));

    mi.Memory.popScope(true);

    // Char

    mi.Memory.pushScope("char");
    mi.LoadedModules.insert("char");

    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeChar),
            "toUpper",
            std::map<std::string, LanType>{ { "self", LanType(LanTypeEnum::TypeChar) } },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                return LanVariable::Char(std::tolower(static_cast<unsigned char>(std::get<char>(args[0].Value))));
            }
        )
    ));

    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeChar),
            "toLower",
            std::map<std::string, LanType>{ { "self", LanType(LanTypeEnum::TypeChar) } },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                return LanVariable::Char(std::tolower(static_cast<unsigned char>(std::get<char>(args[0].Value))));
            }
        )
    ));

    mi.Memory.popScope(true);
}
