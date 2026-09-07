// ModuleRegistry.cpp
#include "LanVariable.h"
#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include <variant>

void ModuleRegistry::RegisterHardwires(MylangeInterpreter& mi)
{
    // nil exit ()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "exit",
            LanFunction::ParamStruct{ },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                throw std::runtime_error("[exit]");
            }
        )
    ));

    // any return (any: o)
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeAny),
            "return",
            LanFunction::ParamStruct{ { "o", LanType(LanTypeEnum::TypeAny) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (args.empty()) throw std::runtime_error("print requires 1 argument.");
                return args[0];
            }
        )
    ));

    // array<T> empty(type: T)
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeType),
            "empty",
            LanFunction::ParamStruct{ { "T", LanType(LanTypeEnum::TypeType) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (args.empty()) throw std::runtime_error("empty requires 1 argument.");
                auto& t = std::get<LanType>(args[0]->Value);
                return std::make_shared<LanVariable>(LanVariable(t, {}));
            }
        )
    ));

    // type ty(str: typeStr)
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeType),
            "ty",
            LanFunction::ParamStruct{ { "typeStr", LanType(LanTypeEnum::TypeString) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (args.empty()) throw std::runtime_error("ty requires 1 argument.");
                auto t = LanType::FromString(std::get<std::string>(args[0]->Value));
                return std::make_shared<LanVariable>(LanType(LanTypeEnum::TypeType), t);
            }
        )
    ));

    // func f_find (str: name, arr<str> param_types)
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeFunction),
            "f_find",
            LanFunction::ParamStruct{
                { "name", LanType(LanTypeEnum::TypeString) },
                { "param_types", LanType(LanTypeEnum::TypeType | LanTypeEnum::TypeArray) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (args.empty()) throw std::runtime_error("f_find requires 2 arguments.");
                std::vector<LanType> param_types = {};
                for (auto& param_type_str : std::get<LanArray>(args[1]->Value)) {
                    if (param_type_str->Type != LanTypeEnum::TypeType) throw runtime_error("Expected Type, got " + param_type_str->Type.ToString());
                    param_types.push_back(std::get<LanType>(param_type_str->Value));
                }
                auto f = mi.FindFunction(std::get<std::string>(args[0]->Value), param_types);

                return std::make_shared<LanVariable>(LanType(LanTypeEnum::TypeFunction), LanVariable::LanValue{ f });
            }
        )
    ));

    // type typeof (o: any)
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeType),
            "typeof",
            LanFunction::ParamStruct{ { "o", LanType(LanTypeEnum::TypeAny) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (args.empty()) throw std::runtime_error("typeof requires 1 argument.");
                return std::make_shared<LanVariable>(
                    LanType(LanTypeEnum::TypeType),
                    LanVariable::LanValue{ args[0]->Type }
                );
            }
        )
    ));

    // str sizeof (o: any)
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeInt),
            "sizeof",
            LanFunction::ParamStruct{ { "o", LanType(LanTypeEnum::TypeAny) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (args.empty()) throw std::runtime_error("sizeof requires 1 argument.");
                return std::make_shared<LanVariable>(
                    LanType(LanTypeEnum::TypeInt),
                    LanVariable::LanValue{ static_cast<int>(sizeof(args[0]->Value)) }
                );
            }
        )
    ));

    // nil debug ()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "debug",
            LanFunction::ParamStruct{ },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                CommandLineInterface::DebugEnabled = !CommandLineInterface::DebugEnabled;
                return std::nullopt;
            }
        )
    ));

    // nil debug (bool: o)
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "debug",
            LanFunction::ParamStruct{ { "o", LanType(LanTypeEnum::TypeBool) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (!args.empty())
                    CommandLineInterface::DebugEnabled = std::get<bool>(args[0]->Value);
                return std::nullopt;
            }
        )
    ));

    // ##############
	// Integers <int>
    // ##############

    mi.Memory.pushScope("int");
    mi.LoadedModules.insert("int");

    // nil _Operator>>()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "_Operator>>",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeInt) }
            },
            [&](std::vector<std::shared_ptr<LanVariable>> args) -> std::optional<std::shared_ptr<LanVariable>> {

                args[0]->Value = std::get<int>(args[0]->Value) + 1;

                CommandLineInterface::DebugPrint(std::format("ptr address (from >>): {}", static_cast<void*>(args[0].get())));

                return nullopt;
            }
        )
    ));

    // nil _Operator>>(int: amount)
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "_Operator>>",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeInt) },
			    {"amount", LanType(LanTypeEnum::TypeInt) }
            },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {

                args[0]->Value = std::get<int>(args[0]->Value) + std::get<int>(args[1]->Value);

                return nullopt;
            }
        )
    ));

    // nil _Operator<<()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "_Operator<<",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeInt) }
            },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {

                auto selfv = std::get<int>(args[0]->Value);

                args[0]->Value = selfv -= 1;

                return nullopt;
            }
        )
    ));

    // nil _Operator<<(int: amount)
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "_Operator<<",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeInt) },
                {"amount", LanType(LanTypeEnum::TypeInt) }
            },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {

                auto selfv = std::get<int>(args[0]->Value);
                auto amount = std::get<int>(args[1]->Value);

                args[0]->Value = selfv -= amount;

                return nullopt;
            }
        )
    ));

    mi.Memory.popScope(true);

    // #############
    // Strings <str>
    // #############
    mi.Memory.pushScope("str");
    mi.LoadedModules.insert("str");

    // str <str>.toUpper()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "toUpper",
            LanFunction::ParamStruct{ { "self", LanType(LanTypeEnum::TypeString) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                string text = std::get<string>(args[0]->Value);
                for (char& c : text) {
                    c = std::toupper(static_cast<unsigned char>(c));
                }
                return std::make_shared<LanVariable>(LanVariable::String(text));
            }
        )
    ));

    // str <str>.toLower()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeString),
            "toLower",
            LanFunction::ParamStruct{ { "self", LanType(LanTypeEnum::TypeString) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                string text = std::get<string>(args[0]->Value);
                for (char& c : text) {
                    c = std::tolower(static_cast<unsigned char>(c));
                }
                return std::make_shared<LanVariable>(LanVariable::String(text));
            }
        )
    ));

    // str <str>.at(int: index)
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeChar),
            "at",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeString) },
                { "index", LanType(LanTypeEnum::TypeInt) }},
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                string text = std::get<string>(args[0]->Value);
                return std::make_shared<LanVariable>(LanVariable::Char(text[std::get<int>(args[1]->Value)]));
            }
        )
    ));

    // str <str>.format(array<any> values)
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeChar),
            "format",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeString) },
                { "index", LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                string text = std::get<string>(args[0]->Value);
				auto& values = std::get<LanArray>(args[1]->Value);
                // get position of all % (without \) in the string
				size_t pos = 0;
				for (int i = 0; i < values.size(); ++i) {
					pos = text.find("%", pos);
					if (pos == string::npos) break;
					text.replace(pos, 1, values[i]->ToString());
					pos += values[i]->ToString().length();
				}
                return std::make_shared<LanVariable>(LanVariable::String(text));
            }
        )
    ));

    // contains
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeBool),
            "contains",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeString) },
                { "querry", LanType(LanTypeEnum::TypeString) } },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                return std::make_shared<LanVariable>(LanVariable::Bool(std::get<string>(args[0]->Value)
                    .find(std::get<string>(args[1]->Value)) != std::string::npos));
            }
        )
    ));

    // int _Count()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeInt),
            "_Count",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeString) },
            },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                return std::make_shared<LanVariable>(LanVariable::Int(static_cast<int>(std::get<string>(args[0]->Value).length())));
            }
        )
    ));

    mi.Memory.popScope(true);

    // ###########
    // Char <char>
    // ###########

    mi.Memory.pushScope("char");
    mi.LoadedModules.insert("char");

    // char <char>.toUpper()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeChar),
            "toUpper",
            LanFunction::ParamStruct{ { "self", LanType(LanTypeEnum::TypeChar) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                return std::make_shared<LanVariable>(
                    LanVariable::Char(std::toupper(static_cast<unsigned char>(std::get<char>(args[0]->Value)))));
            }
        )
    ));

    // char <char>.toLower()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeChar),
            "toLower",
            LanFunction::ParamStruct{ { "self", LanType(LanTypeEnum::TypeChar) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                return std::make_shared<LanVariable>(
                    LanVariable::Char(std::tolower(static_cast<unsigned char>(std::get<char>(args[0]->Value)))));
            }
        )
    ));

    // int _Count()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeInt),
            "_Count",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeChar) },
            },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                return std::make_shared<LanVariable>(
                    LanVariable::Int(static_cast<uint32_t>(std::get<char>(args[0]->Value))));
            }
        )
    ));

    mi.Memory.popScope(true);

    // ####################
    // Functions <function>
    // ####################

    mi.Memory.pushScope("function");
    mi.LoadedModules.insert("function");

    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanTypeEnum::TypeAny,
            "do",
            LanFunction::ParamStruct {
                { "self", LanType(LanTypeEnum::TypeFunction) },
                { "args", LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                auto& func = std::get<shared_ptr<LanFunction>>(args[0]->Value);
                LanArray fargs = {};
                for (auto& u : std::get<LanArray>(args[1]->Value)) {
                    fargs.push_back(u);
                }

                return func->Execute(mi, fargs);
            }
        )
    ));
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanTypeEnum::TypeAny,
            "do",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeFunction) } },
                [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                auto& func = std::get<shared_ptr<LanFunction>>(args[0]->Value);
                return func->Execute(mi, {});
            }
        )
    ));

    mi.Memory.popScope(true);

    // ##############
	// Arrays <array>
    // ##############

    mi.Memory.pushScope("array");
    mi.LoadedModules.insert("array");

    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny),
            "where",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny) },
                { "func", LanType(LanTypeEnum::TypeFunction) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {

				std::vector<std::shared_ptr<LanVariable>> result;

				auto& array = std::get<LanArray>(args[0]->Value);
				auto& func = std::get<shared_ptr<LanFunction>>(args[1]->Value);

				for (auto& item : array) {
					auto res = func->Execute(mi, { item });
					if (res.has_value() && res.value()->Type == LanTypeEnum::TypeBool && std::get<bool>(res.value()->Value)) {
						result.push_back(item);
					}
				}

                return std::make_shared<LanVariable>(LanVariable::Array(result));
            }
        )
    ));

    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny),
            "for",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny) },
                { "func", LanType(LanTypeEnum::TypeFunction) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {

                std::vector<std::shared_ptr<LanVariable>> result;

                auto& array = std::get<LanArray>(args[0]->Value);
                auto& func = std::get<shared_ptr<LanFunction>>(args[1]->Value);

                for (auto& item : array) {
                    auto res = func->Execute(mi, { item });
                    if (res.has_value()) {
                        result.push_back(res.value());
                    }
                }

                return std::make_shared<LanVariable>(LanVariable::Array(result));
            }
        )
    ));

    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny),
            "range",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny) },
                { "start", LanType(LanTypeEnum::TypeInt) },
                { "end", LanType(LanTypeEnum::TypeInt) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                if (args[0]->Type != LanType(LanTypeEnum::TypeArray) &&
                    args[0]->Type != LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny) &&
                    args[0]->Type != LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeInt)) {
                    throw runtime_error("Array needs to accept 'int' for range function.");
                }
                auto& array = std::get<LanArray>(args[0]->Value);

                auto& start = std::get<int>(args[1]->Value);
                auto& end   = std::get<int>(args[2]->Value);

                if (start >= end) throw runtime_error(std::format("Start must be less than the end: {} >= {}", start, end));

                for (int i = start; i < end; i++) {
                    array.push_back(std::make_shared<LanVariable>(LanVariable::Int(i)));
                }

                return args[0];
            }
        )
    ));

    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "append",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny) },
                { "item", LanType(LanTypeEnum::TypeAny) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {

                auto& array = std::get<LanArray>(args[0]->Value);

				if (!LanVariable::IsAppendable(args[0]->Type, args[1]->Type))
					throw runtime_error("Cannot append item of type " + args[1]->Type.ToString() + " to array of type " + args[0]->Type.ToString());

				array.push_back(args[1]);

                return nullopt;
            }
        )
    ));

    // nil <array>._Operator<<()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeNil),
            "_Operator<<",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny) },
                { "item", LanType(LanTypeEnum::TypeAny) } },
            [&](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {

                auto& array = std::get<LanArray>(args[0]->Value);

                if (!LanVariable::IsAppendable(args[0]->Type, args[1]->Type))
                    throw runtime_error("Cannot append item of type " + args[1]->Type.ToString() + " to array of type " + args[0]->Type.ToString());

                array.push_back(args[1]);

                return nullopt;
            }
        )
    ));

    // int _Count()
    mi.Memory.define(std::make_shared<LanVariable>(
        LanType(LanTypeEnum::TypeFunction),
        std::make_shared<BuiltinFunction>(
            LanType(LanTypeEnum::TypeInt),
            "_Count",
            LanFunction::ParamStruct{
                { "self", LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny) },
            },
            [](LanArray args) -> std::optional<std::shared_ptr<LanVariable>> {
                return std::make_shared<LanVariable>(LanVariable::Int(static_cast<int>(std::get<LanArray>(args[0]->Value).size())));
            }
        )
    ));

    mi.Memory.popScope(true);
}
