// ModuleRegistry.cpp
#include "LanVariable.h"
#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include <variant>

void ModuleRegistry::RegisterHardwires(MylangeInterpreter& mi)
{
    // nil exit ()
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

    // any return (any: o)
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

    // array<T> empty(type: T)
    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeType),
            "empty",
            std::map<std::string, LanType>{ { "T", LanType(LanTypeEnum::TypeType) } },
            [](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (args.empty()) throw std::runtime_error("empty requires 1 argument.");
                auto& t = std::get<LanType>(args[0].Value);
                return LanVariable(t, {});
            }
        )
    ));

    // type ty(str: typeStr)
    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeType),
            "ty",
            std::map<std::string, LanType>{ { "typeStr", LanType(LanTypeEnum::TypeString) } },
            [](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (args.empty()) throw std::runtime_error("ty requires 1 argument.");
                auto t = LanType::FromString(std::get<std::string>(args[0].Value));
                return LanVariable(LanType(LanTypeEnum::TypeType), t);
            }
        )
    ));

    // func f_find (str: name, arr<str> param_types)
    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeFunction),
            "f_find",
            std::map<std::string, LanType>{
                { "name", LanType(LanTypeEnum::TypeString) },
                { "param_types", LanType(LanTypeEnum::TypeString | LanTypeEnum::TypeArray) } },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                if (args.empty()) throw std::runtime_error("f_find requires 2 arguments.");
                std::vector<LanType> param_types = {};
                try {
                    for (auto& param_type_str : std::get<LanArray>(args[1].Value)) {
                        if (param_type_str->Type != LanTypeEnum::TypeString) throw runtime_error("Expected string, got " + param_type_str->Type.ToString());
                        param_types.push_back(LanType::FromString(std::get<std::string>(param_type_str->Value)));
                    }
                }
                catch (exception& e) { }
                
                auto f = mi.FindFunction(std::get<std::string>(args[0].Value), param_types);

                return LanVariable(LanType(LanTypeEnum::TypeFunction), LanVariable::LanValue{ f });
            }
        )
    ));

    // str typeof (o: any)
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

    // str sizeof (o: any)
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

    // nil debug ()
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

    // nil debug (bool: o)
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

    // Strings <str>
    mi.Memory.pushScope("str");
    mi.LoadedModules.insert("str");

    // str <str>.toUpper()
    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "toUpper",
            std::map<std::string, LanType>{ { "self", LanType(LanTypeEnum::TypeString) } },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                string text = std::get<string>(args[0].Value);
                for (char& c : text) {
                    c = std::toupper(static_cast<unsigned char>(c));
                }
                return LanVariable::String(text);
            }
        )
    ));

    // str <str>.toLower()
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

    // str <str>.at(int: index)
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

    // Char <char>

    mi.Memory.pushScope("char");
    mi.LoadedModules.insert("char");

    // char <char>.toUpper()
    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeChar),
            "toUpper",
            std::map<std::string, LanType>{ { "self", LanType(LanTypeEnum::TypeChar) } },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                return LanVariable::Char(std::toupper(static_cast<unsigned char>(std::get<char>(args[0].Value))));
            }
        )
    ));

    // char <char>.toLower()
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

    // Functions <function>

    mi.Memory.pushScope("function");
    mi.LoadedModules.insert("function");

    mi.Memory.define(LanVariable(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeAny),
            "do",
            std::map<std::string, LanType>{
                { "self", LanType(LanTypeEnum::TypeFunction) },
                { "args", LanType(LanTypeEnum::TypeArray) } },
            [&](std::vector<LanVariable> args) -> std::optional<LanVariable> {
                auto& func = std::get<shared_ptr<LanFunction>>(args[0].Value);
                vector<LanVariable> fargs = {};
                for (auto& u : std::get<LanArray>(args[1].Value)) {
                    fargs.push_back(*u);
                }

                return func->Execute(mi, fargs);
            }
        )
    ));

    mi.Memory.popScope(true);
}
