// IMPORTS //
#include "CommandLineInterface.h"
#include "LanArithmetic.h"
#include "LanClass.h"
#include "LanIterableEngine.h"
#include "LanType.h"
#include "LanVariable.h"
#include "ModuleRegistry.h"
#include "MylangeInterpreter.h"
#include "Utils.h"
#include "MylangeFileInterface.h"
#include <format>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// General Regexes
const regex functionPartsPattern(R"(^([\w.]+)\s*\((.*)\))", std::regex_constants::ECMAScript);
const regex singleFunctionPartPattern(R"(^(\w+)\s*\((.*)\)$)", std::regex_constants::ECMAScript);
const regex functionCallStack(R"(^(?:\w+\.?)+(?:\w+\(.*\))+)", std::regex_constants::ECMAScript);
const regex functionDotExtention(R"((\)\.))", std::regex_constants::ECMAScript);
const regex ifElseThenPattern(R"(^if\s*\((.*?)\)\s*then\s*(.*))", std::regex_constants::ECMAScript);
const regex paramStringPattern(R"((?:(const)\s+)?([\w<|>.]+)\s+(\w+))", std::regex_constants::ECMAScript);
const regex functionMethodDeclaration(R"(^((?:@?\w+\s+)+)?def\s+(\w+)\s+(\w+)\s*\((.*)\)\s*as\s*(.*))", std::regex_constants::ECMAScript);
const regex classDeclarationPatter(R"(^class\s+(\w+)\s+(?:extends\s+(\w+))?\s*has\s*(.*))", std::regex_constants::ECMAScript);
const regex wordCharsOnly(R"(^[a-zA-Z]\w*$)", std::regex_constants::ECMAScript);
const regex cachedBit(R"((\d)x([a-fA-F0-9]+))", std::regex_constants::ECMAScript);
const regex lambdaPattern(R"(^\[([\w<|>.]+)\]\s*\(([\w<|>. ,]*)\)\s*(?:->|as)\s*(.*))", std::regex_constants::ECMAScript);
const regex setVariable(R"(^\s*([a-zA-Z<>,|\s.]+) +(\w+) *=> *(.*))");

// For Classes
const regex classDefualtPropertyPatter(R"(^((?:@?\w+\s+)+)?\s*([a-zA-Z<>,|\s]+) +(\w+) *=> *(.*))", std::regex_constants::ECMAScript);
const regex classPropertyPattern(R"(^((?:@?\w+\s+)+)?\s*([a-zA-Z<>,|\s]+) +(\w+))", std::regex_constants::ECMAScript);
const regex castingCreationPattern(R"(^new\s+([\w.]+)\((.*)\))", std::regex_constants::ECMAScript);
const std::string overrideString = std::string("@override");

struct Rule {
    regex pattern;
    function <optional<LanVariable> (const smatch&, MylangeInterpreter&) > action;
};

vector<Rule> rules = {
    {
        regex(R"(^break)"),
        [](auto const& m, MylangeInterpreter& mi) {
            CommandLineInterface::DebugPrint("Break statement called.");
            return optional<LanVariable>(LanVariable(LanType(LanTypeEnum::TypeNil), 1));
        }
    },
    
    // Include (modules)
    {
        regex(R"(^#\s*include\s*<(\w+)>)"),
        [](auto const& m, MylangeInterpreter& mi) {
            std::string moduleName = m[1].str();

            if (mi.LoadedModules.contains(moduleName)) {
                CommandLineInterface::DebugPrint("Module already loaded: " + moduleName);
                return std::nullopt;
            }

            if (!ModuleRegistry::Has(moduleName))
                throw std::runtime_error("Unknown module: " + moduleName);

            // Push module scope to register functions into it
            mi.Memory.pushScope(moduleName);
            std::string moduleScopeId = mi.Memory.currentScope()->id;
            ModuleRegistry::Load(moduleName, mi, moduleScopeId);
            mi.Memory.popScope(true); // immediately return to the calling scope

            mi.LoadedModules.insert(moduleName);
            CommandLineInterface::DebugPrint("Loaded module: " + moduleName + " into scope: " + moduleScopeId);
            return std::nullopt;
        }
    },
    // Include (Local file)
    {
        regex(R"(^#\s*include\s*(1x[0-9a-fA-F]{8})\s*(?:as\s*<(\w+)>)?$)"),
        [](auto const& m, MylangeInterpreter& mi) {
            std::string moduleName = mi.BlockMap[m[1].str()];
            std::string asName = (m[2].matched) ? m[2].str() : moduleName;

            CommandLineInterface::DebugPrint("Loading local module '" + moduleName + "' as '" + asName + "'");

            if (mi.LoadedModules.contains(asName)) throw runtime_error("Cannot import module, already exists: " + moduleName + " as " + asName);
            mi.LoadedModules.insert(asName);

            mi.Memory.pushScope(asName);

            auto content = FileInterface::CleanFile(moduleName + ".myl");
            mi.InterpretBlock(content);

            mi.Memory.popScope(true);

            

            return std::nullopt;
        }
    },
	// Continue: loads a script into the current scope. Used to allow a file to "continue" to another file, then come back.
    {
        regex(R"(^#\s*continue\s*(1x[0-9a-fA-F]{8}))"),
        [](auto const& m, MylangeInterpreter& mi) {
            std::string moduleName = mi.BlockMap[m[1].str()];

            auto content = FileInterface::CleanFile(moduleName + ".myl");
            mi.InterpretBlock(content);

            return std::nullopt;
        }
    },
    // Set Variable
    {
        regex(R"(^\s*([a-zA-Z<>,|\s.]+) +(\w+) *=> *(.*))"),
        [](auto const& m, MylangeInterpreter& mi) {
            string expected_type_name = Utils::TrimString(m[1]);
            LanType expectedType = mi.ResolveType(expected_type_name);

            CommandLineInterface::DebugPrint("EXP TYPE:" + to_string(static_cast<uint32_t>(expectedType.BaseType)) + " / " + expectedType.ToString());
            auto lv = mi.ParseParameter(m[3]);
            if (!lv.has_value())
                throw runtime_error("Failed to parse variable value.");
            CommandLineInterface::DebugPrint("Setting variable " + m[2].str() + " of type " + expectedType.ToString()
                + "/" + lv.value().Type.ToString() + " to value " + lv.value().ToString());
            if (lv.value().IsCompatible(expectedType)) {

                if (expectedType.IsArrayType() && lv.value().Type == LanTypeEnum::TypeArray) {
                    // Override empty arrays
                    lv.value().Type = expectedType;
                }

                mi.Memory.define(m[2].str(), lv.value());
            }
                
            else throw runtime_error("Type mismatch in variable assignment. Expected "
                + expectedType.ToString() + ", got " + lv.value().Type.ToString()
                + " (with " + lv.value().ToString() + ")");
            return nullopt;
			
        }
    },
    // Reset variable
    {
        regex(R"(^\s*(\w[\w :\[\]]*?)\s*=>\s*(.*)$)"),
        [](auto const& m, MylangeInterpreter& mi) {
            string place_to_write = m[1].str();
            auto tokens = mi.TokenizeComplexValue(place_to_write);


            LanVariable* working = nullptr;
            for (auto& token : tokens) {
                CommandLineInterface::DebugPrint(std::format("Working on token: '{}': {}", token.value, token.type_string()), 1);
                switch (token.type) {
                case TokenItem::Value:
                    // Resolve first what variable is being called
                    working = mi.Memory.resolve(token.value);
                    break;
                case TokenItem::ColonExtention:
					if (!working) throw runtime_error("Cannot use colon extension on non-variable.");


                    if (working->HasIndex(token.value)) {
                        working = working->Index(token.value).get();
                    }
                    else if (working->Type.IsTable()) {
                        auto& table = std::get<LanTable>(working->Value);
                        table.Place(token.value, std::make_shared<LanVariable>(LanVariable::Nil()));
                        working = working->Index(token.value).get();
                    }
                    else if (working->Type.IsSetType()) {
                        throw out_of_range("Cannot add key to set.");
                    }
                    else throw runtime_error("HERE");


                    break;
                case TokenItem::BracketExtention:
                    if (!working) throw runtime_error("Cannot use bracket extension on non-variable.");
                    auto indexValueOpt = mi.ParseParameter(token.value);
                    if (!indexValueOpt.has_value()) throw runtime_error("No value for bracket-extension.");
                    auto& indexValue = indexValueOpt.value();
                    if (indexValue.Type == LanTypeEnum::TypeInt) {
                        working = working->Index(std::get<int>(indexValue.Value)).get();
                    }
                    else if (indexValue.Type == LanTypeEnum::TypeString) {
                        auto& key = std::get<std::string>(indexValue.Value);

                        if (working->HasIndex(key)) {
                            working = working->Index(key).get();
                        }
                        else if (working->Type.IsTable()) {
                            auto& table = std::get<LanTable>(working->Value);
                            table.Place(key, std::make_shared<LanVariable>(LanVariable::Nil()));
                            working = working->Index(key).get();
                        }
                        else if (working->Type.IsSetType()) {
                            throw out_of_range("Cannot add key to set.");
                        }
                        else throw runtime_error("HERE");


                    }
                    else throw runtime_error(std::format("Cannot bracket-index with type: {} [WIP]", indexValue.Type.ToString()));

                    break;
                }
            }
            // Now that the pointer is found, we can re-assign the value
            if (!working) throw runtime_error("Failed to resolve pointer path.");
            auto new_value = mi.ParseParameter(m[2].str());
            if (new_value.has_value()) {
                *working = new_value.value();
            }
            else throw runtime_error("Missing value in reset.");
            return nullopt;
        }
    },
    // Do Block
    {
        regex(R"(^do\s+(.*))"),
        [](auto const& m, MylangeInterpreter& mi) {
			CommandLineInterface::DebugPrint("[" + mi.Memory.currentScope()->id + "] Interpreting block: " + m[1].str());
            mi.Memory.pushScope("do");
			mi.InterpretBlock(m[1]);
            mi.Memory.popScope();
            return nullopt;
        }

    },
    // Cached Block
    {
        regex(R"(^0x([a-fA-F0-9]+))"),
        [](auto const& m, MylangeInterpreter& mi) {
            CommandLineInterface::DebugPrint("Found cached block: " + m[0].str());
            string cached_block = mi.BlockMap[m[0]];
            mi.Memory.pushScope(m[0]);
            auto res = mi.InterpretBlock(cached_block);
            mi.Memory.popScope();
            return res;
        }
    },
    // Class
    {
        classDeclarationPatter,
        [](auto const& m, MylangeInterpreter& mi) -> std::optional<LanVariable> {
       
            // Determine body
            smatch match;
            string body = (regex_match(m[3].str(), cachedBit)) ? mi.BlockMap[m[3].str()] : m[3].str();
			string name = m[1].str();
			string extends = m[2].str();

			CommandLineInterface::DebugPrint("Parsing class: " + name + (extends.empty()?"" : " / Extends: " + extends));

			auto lines = Utils::TopLevelSplit(body, ';');

            std::unordered_map<std::string, LanType> properties;
            std::unordered_map<std::string, LanVariable> defaultValues;
            std::unordered_map<std::string, std::shared_ptr<LanFunction>> methods;

            if (!extends.empty()) {

                auto a = mi.Memory.resolve(extends);
                if (!a) throw runtime_error("Parent class not found: " + extends);
                shared_ptr<LanClass> parentClass = std::get<shared_ptr<LanClass>>(a->Value);
                CommandLineInterface::DebugPrint("Found parent class: " + extends);
                // Inherit properties
                for (auto& [propName, propType] : parentClass->Properties) {
                    properties[propName] = propType;
                }
                // Inherit default values
                for (auto& [propName, defaultValue] : parentClass->DefaultValues) {
                    defaultValues[propName] = defaultValue;
                }
                // Inherit methods
                for (auto& [methodName, method] : parentClass->Methods) {
                    methods[methodName] = method;
                }
			}

            for (auto& rawLine : lines) {
				string line = Utils::TrimString(rawLine);
				if (line.empty()) continue;
                CommandLineInterface::DebugPrint("Parsing class line: " + line);

                if (line == "@default constructor") {
                    if (extends.empty()) throw runtime_error("Cannot define default constructor on non-extended class.");
                    
                    std::smatch old_match;
                    std::regex u(R"((\w+)(\(.*\)))");

                    for (auto it = methods.begin(); it != methods.end(); ) {
                        auto& [methodName, method] = *it;
                        if (!std::regex_match(methodName, old_match, u))
                            throw std::runtime_error("Method does not match correct format?: " + methodName);
                        if (old_match[1].str() == extends) {
                            std::string new_id = name + old_match[2].str();
                            methods[new_id] = method;      // copy value
                            it = methods.erase(it);        // continue from next element
                        }
                        else ++it;
                    }
                    
                }
                else if (regex_match(line, match, classDefualtPropertyPatter)) {
                    CommandLineInterface::DebugPrint("Found default property: " + match[3].str() + " of type " + match[2].str());
                    // Modifiers
					vector<string> modifiers = Utils::TopLevelSplit(Utils::TrimString(match[1].str()), ' ');
                    // Default property
                    string property_name = match[3].str();
                    // Check if override is needed
                    if (((properties.find(property_name) != properties.end()) && !Utils::Find(modifiers, overrideString))
                        || ((defaultValues.find(property_name) != defaultValues.end()) && !Utils::Find(modifiers, overrideString)))
                        throw runtime_error("Cannot declare the same property with default without override.");
                    LanType property_type = LanType::FromString(Utils::TrimString(match[2].str()));
                    string default_value_str = match[4].str();
                    auto default_value = mi.ParseParameter(default_value_str);
                    if (!default_value.has_value()) throw runtime_error("Failed to parse default value for class property.");
                    if (!default_value.value().IsCompatible(property_type)) throw runtime_error("Default value type mismatch for class property.");

					defaultValues[property_name] = move(default_value.value());
                    properties[property_name] = property_type;
                }
                else if (regex_match(line, match, classPropertyPattern)) {
                    CommandLineInterface::DebugPrint("Found property: " + match[3].str() + " of type " + match[2].str());
                    // Modifiers
                    vector<string> modifiers = Utils::TopLevelSplit(Utils::TrimString(match[1].str()), ' ');
                    // Regular property
                    string property_name = match[3].str();
                    // Check if override is needed
                    if ((properties.find(property_name) != properties.end()) && !Utils::Find(modifiers, overrideString))
                        throw runtime_error("Cannot declare the same property without override.");
                    LanType property_type = LanType::FromString(Utils::TrimString(match[2].str()));
					properties[property_name] = property_type;
                }
				else if (regex_match(line, match, functionMethodDeclaration)) {
                    CommandLineInterface::DebugPrint("Found method: " + match[3].str() + " of type " + match[2].str());
                    // Modifiers
                    vector<string> modifiers = Utils::TopLevelSplit(Utils::TrimString(match[1].str()), ' ');
                    // Method
                    string method_name = match[3].str();
                    LanType return_type = LanType::FromString(Utils::TrimString(match[2].str()));
                    LanFunction::ParamStruct parameter_map;
                    for (const auto& param_str : Utils::TopLevelSplit(match[4], ',')) {
                        auto parts = Utils::TopLevelSplit(Utils::TrimString(param_str), ' ');
                        if (parts.size() != 2) throw runtime_error("Each param must have 2 parts. Found " + to_string(parts.size()));
                        parameter_map.push_back({ parts[1], LanType::FromString(parts[0]) });
                    }
                    string logic = match[5].str();
                    unique_ptr<LanFunction> method =
                        make_unique<ScriptFunction>(return_type, method_name, parameter_map, logic);
                    // Check if override is needed
                    if ((methods.find(method->GetId()) != methods.end()) && !Utils::Find(modifiers, overrideString))
                        throw runtime_error("Cannot declare the same method without override.");
					methods[method->GetId()] = move(method);
                }
				else throw runtime_error("Invalid class body line: '" + line + "'");
            }

			mi.Memory.define(name, LanVariable(LanType(LanTypeEnum::TypeClass), make_shared<LanClass>(name, properties, defaultValues, methods)));

            return nullopt;
        }
    },
    // Functions
    {
        functionMethodDeclaration,
        [](auto const& m, MylangeInterpreter& mi) {
            // Settup params
            LanFunction::ParamStruct parameter_map;
            for (const auto& param_str : Utils::TopLevelSplit(m[4], ','))
            {
                auto parts = Utils::TopLevelSplit(Utils::TrimString(param_str), ' ');
                if (parts.size() != 2) throw runtime_error("Each param must have 2 parts. Found " + to_string(parts.size()));
                parameter_map.push_back({ parts[1] , LanType::FromString(parts[0]) });
            }
            const LanType return_type = LanType::FromString(m[2]);
            std::shared_ptr<LanFunction> function =
                make_unique<ScriptFunction>(return_type, m[3].str(), parameter_map, m[5].str());
			CommandLineInterface::DebugPrint("[" + mi.Memory.currentScope()->id + "] Registering function : " + function->GetId() + " with return type " + return_type.ToString());
            
			mi.Memory.define(function->GetId(), LanVariable(LanType(LanTypeEnum::TypeFunction), function));
            return nullopt;
        }
    },
    // Functional Execution
    {
        functionCallStack,
        [](auto const& m, MylangeInterpreter& mi) {
            auto _ = mi.ParseParameter(m[0].str());
            return nullopt;
        }
    },
    // If/Else/Then Block
    {
        ifElseThenPattern,
        [](auto const& m, MylangeInterpreter& mi) -> std::optional<LanVariable> {
			auto parts = Utils::SplitString(m[0], "else");
			for (auto& part : parts) {
                CommandLineInterface::DebugPrint("Running If/Else/Then line: " + part);
                part = Utils::TrimString(part);
                smatch match;
                if (regex_match(part, match, ifElseThenPattern))
                {
					auto conditionEval = mi.ParseParameter(m[1].str());

                    if (conditionEval.has_value() && ((conditionEval.value().Type.BaseType & LanTypeEnum::TypeBool) == LanTypeEnum::TypeBool)
                        && get<bool>(conditionEval.value().Value))
                    {
						mi.Memory.pushScope("if"); // Enter if scope
                        auto out = mi.InterpretBlock(match[2].str());
						mi.Memory.popScope(); // Exit if scope
                        if (out.has_value()) {
                            CommandLineInterface::DebugPrint("If has return value: " + out.value().Type.ToString());
                            return out;
                        }
                    }
                }
                else {
					mi.Memory.pushScope("if"); // Enter if scope
                    auto out = mi.InterpretBlock(part);
					mi.Memory.popScope(); // Exit if scope
                    if (out.has_value())
                        return out;
                }
            }
            return nullopt;
        }
    },
    // For loop
    {
        regex(R"(for\s*\((.*)\)\s*do\s*(.*))"),
        [](auto const& m, MylangeInterpreter& mi) {
			CommandLineInterface::DebugPrint("Interpreting for loop: " + m[0].str());
            // Get the iter variable to loop over

            auto iter = mi.ParseParameter(m[1].str());


            if (!iter.has_value()) throw runtime_error("Cannot use undefined value for looping.");
            auto& ie = std::get<std::shared_ptr<LanIterableEngine>>(iter.value().Value);

            CommandLineInterface::DebugPrint("This iter has " + to_string(ie->Values.size()) + " value(s)");

            for (auto& valueVariant : ie->Values) {
                mi.Memory.pushScope("for"); // Enter for scope
                // Create the parameters as variables inside the loop
                visit([&](const auto& value) {
                    using T = decay_t<decltype(value)>;
                    if constexpr (is_same_v<T, LanArray>) {
                        auto& value_vector = get<LanArray>(valueVariant);
                        for (int i = 0; i < value_vector.size(); i++) {
                            auto& value = value_vector[i];
                            if (!value->IsCompatible(ie->Keys[i].second)) throw runtime_error("Type expected and given mismatch.");
							mi.Memory.define(ie->Keys[i].first, *value);
                        }
                    }
                    else if constexpr (is_same_v<T, LanVariable>) {
                        auto& value = get<LanVariable>(valueVariant);
                        if (!value.IsCompatible(ie->Keys[0].second)) throw runtime_error("Type expected and given mismatch.");
						mi.Memory.define(ie->Keys[0].first, value);
                    };
                    
                    }, valueVariant);
                // Run the logic of the loop.
                
                auto out = mi.InterpretBlock(m[2].str());
                mi.Memory.popScope(); // Exit for scope

                if (out.has_value()) break;
            }

            return nullopt;
        }
    },
    // While loop
    {
        regex(R"(^while\s*\((.*)\)\s*do\s*(.*))"),
        [](auto const& m, MylangeInterpreter& mi) {
            bool running = true;
            while (running) {
                auto condition_var = mi.ParseParameter(m[1].str());
                if (!condition_var.has_value()) throw runtime_error("Cannot use undefined in while loop.");
                if (condition_var.value().Type != LanType(LanTypeEnum::TypeBool))
                    throw runtime_error("Must use boolean statement in while loop. Got " + condition_var.value().Type.ToString());
                running = std::get<bool>(condition_var.value().Value);
                

                mi.Memory.pushScope("while"); // Enter while scope
                auto out = mi.InterpretBlock(m[2].str());
                mi.Memory.popScope(); // Exit while scope
                
                if (out.has_value()) {
                    CommandLineInterface::DebugPrint("Return value found. Stopping execution");
                    running = false;
                }
            }
            return nullopt;
        }
    },
    // Return
    {
        regex(R"(return\s+(.*))"),
        [](auto const& m, MylangeInterpreter& mi) {
            auto it = mi.ParseParameter(m[1].str());
            if (it.has_value())
                return optional<LanVariable>(it.value());
            throw runtime_error("Trying to return nothing");
        }
    }
};

// CODE //
MylangeInterpreter::MylangeInterpreter()
{
    this->BlockCounter = 1;
};



optional<LanVariable> MylangeInterpreter::Interpret(const string & code)
{
    smatch match;
    string clean_code = Utils::TrimString(code);
    for (const auto& rule : rules) {
        if (regex_search(clean_code, match, rule.pattern)) {
            auto res = rule.action(match, *this);
            if (res.has_value())
                CommandLineInterface::DebugPrint("Line looker returned: " + res.value().Type.ToString() + " / " + res.value().ToString() + " from " + code);
            return res;
        }
    }
    return nullopt;
}


using EscapeMap = std::unordered_map<std::string, char>;

EscapeMap escapes = {
    { "\\\"", '"'  },
    { "\\'",  '\'' },
    { "\\n",  '\n' },
    { "\\t",  '\t' }
};

static std::string CondenseQuotedBlocks(
    const std::string& input,
    char quote,
    const std::string& prefix,
    std::unordered_map<std::string, std::string>* map,
    size_t& counter,
    const EscapeMap& escapeMap = escapes
) {
    std::string output;
    size_t i = 0;

    while (i < input.size()) {
        if (input[i] == quote) {
            size_t start = i + 1;
            size_t j = start;
            std::string inner;

            while (j < input.size()) {
                // Handle escape sequences
                if (input[j] == '\\' && j + 1 < input.size()) {
                    std::string esc = input.substr(j, 2);

                    auto it = escapeMap.find(esc);
                    if (it != escapeMap.end()) {
                        inner += it->second;
                        j += 2;
                        continue;
                    }

                    // Unknown escape  keep literal char
                    inner += input[j + 1];
                    j += 2;
                    continue;
                }

                // Closing quote
                if (input[j] == quote) {
                    break;
                }

                inner += input[j];
                j++;
            }

            if (j >= input.size()) {
                throw std::runtime_error("Unterminated quoted string");
            }

            // Generate replacement code
            std::string code = Utils::MakeHexCode(prefix, counter++);
            (*map)[code] = inner;

            output += code;
            i = j + 1; // skip closing quote
        }
        else {
            output += input[i];
            i++;
        }
    }

    return output;
}

const regex tablePairPattern(R"(^\s*\w+\s*=>.*$)", std::regex_constants::ECMAScript);

static std::pair<bool, std::string> IsTableBlock(const std::string& inner) {
	// If the inner block is empty, it should be interpeted as a table block.
	if (inner.empty()) return { true, inner };
	// First, check for the "Force code block" prefix, {^ ... }
	if (inner[0] == '^') return { false, inner.substr(1) };
    // Then, slip top level ';', auto fails the check
    if (Utils::TopLevelSplit(inner, ';').size() > 1) return { false, inner };
    // Check all comma seperated parts (or the sole part)
	for (const auto& part : Utils::TopLevelSplit(inner, ',')) {
		// If any part is not a valid variable name, it is a code block
		if (!regex_match(Utils::TrimString(part), tablePairPattern)) return { false, inner };
	}
	return { true, inner };
}

static string CondenseBlocks(
    const std::string& input,
    char open,
    char close,
    std::unordered_map<std::string, std::string>* map,
    size_t& counter
) {
    std::string output;
    size_t i = 0;

    while (i < input.size()) {
        if (input[i] == open) {
            // Find matching closing delimiter
            size_t depth = 1;
            size_t start = i + 1;
            size_t j = start;

            while (j < input.size() && depth > 0) {
                if (input[j] == open) depth++;
                else if (input[j] == close) depth--;
                j++;
            }

            // Safety check
            if (depth != 0) {
                throw std::runtime_error("Unmatched block delimiter");
            }

            size_t end = j - 1; // index of closing delimiter

            // Recursively process the block interior
            std::string inner = CondenseBlocks(
                input.substr(start, end - start),
                open, close, map, counter
            );

            // Generate code and store mapping
            // Determine if it is a code block or a table
			auto [isTable, innerContent] = IsTableBlock(inner);
            std::string code = Utils::MakeHexCode(isTable?"3x":"0x", counter++);
            (*map)[code] = innerContent;

            // Replace block with code
            output += code;

            i = j; // continue after closing delimiter
        }
        else {
            output += input[i];
            i++;
        }
    }

    return output;
}

/// <summary>
/// Interprets a block of code, in the current scope.
/// </summary>
/// <param name="blockString"></param>
/// <param name="SkipClearing"></param>
/// <returns></returns>
optional<LanVariable> MylangeInterpreter::InterpretBlock(const string& blockString, const bool SkipClearing)
{
    // Cache stuff
    std::string condensed_block = CondenseQuotedBlocks(blockString, '"', "1x", & this->BlockMap, this->BlockCounter);
    condensed_block = CondenseQuotedBlocks(condensed_block, '\'', "2x", &this->BlockMap, this->BlockCounter);
    condensed_block = CondenseBlocks(
        condensed_block, '{', '}', &this->BlockMap, this->BlockCounter
    );

    // Split into lines
    vector<string> lines = Utils::SplitString(condensed_block, ';');

    for (int i = 0; i < lines.size(); ++i) {
        string line = Utils::TrimString(lines[i]);
        if (line.empty()) continue;
		CommandLineInterface::DebugPrint("[" + this->Memory.currentScope()->id + "] Interpreting line: " + line);
        
        auto res = this->Interpret(line);
        
        if (res.has_value()) {
            CommandLineInterface::DebugPrint("Line returned with a value: " + res.value().Type.ToString());
            return res;
        }
    }

    return nullopt;
}

LanType MylangeInterpreter::ResolveType(const string& typeStr)
{
    LanType result;
    try {
        result = LanType::FromString(typeStr);
    }
    catch (exception& e) {
        auto a = this->Memory.resolve(typeStr);
        if (a)
            result = LanType(std::get<shared_ptr<LanClass>>(a->Value));
        else {
			auto sep = Utils::TopLevelSplit(typeStr, '.');
			if (sep.size() != 2) throw runtime_error(std::format("[{}] Cannot resolve type (too many parts): {}\n\t{}", this->Memory.currentScope()->id, typeStr, e.what()));
			
			auto location = this->Memory.resolveScope(this->Memory.currentScope()->id + "." + sep[0]);
			auto cls = location->resolve(sep[1]);
            if (cls) {
				if (cls->Type != LanTypeEnum::TypeClass) throw runtime_error(std::format("[{}] Resolved type is not a class: {}\n\t{}", this->Memory.currentScope()->id, typeStr, e.what()));
				auto& cls_ptr = std::get<shared_ptr<LanClass>>(cls->Value);
				return LanType(cls_ptr);
            } else throw runtime_error(std::format("[{}] Cannot resolve type: {}\n\t{}", this->Memory.currentScope()->id, typeStr, e.what()));
        }
    };
    return result;
}


const std::vector<std::string> protectedWords = {
    "true", "false", "nil"
};

const regex fullVariablePattern(R"(([a-zA-Z]\w+)((?:(?::\w+)|(?:\[.+?\]))*))", std::regex_constants::ECMAScript);
const regex variableExtentionPattern(R"((?::\w+)|(?:\[.+?\]))", std::regex_constants::ECMAScript);

const regex colonExtention(R"(:(\w+)$)", std::regex_constants::ECMAScript);
const regex bracketExtention(R"(\[(\d+)\]$)", std::regex_constants::ECMAScript);




std::vector<TokenItem> MylangeInterpreter::TokenizeComplexValue(std::string& value)
{
    CommandLineInterface::DebugPrint("Attempting to tokenize: '" + value + "'");

    // Go char-by-char to get all the different possible parts of the value
    DepthEngine depth = DepthEngine();

    vector<TokenItem> parts = {};
    string to_append = "";
    bool possible_package_method = false;
    auto place_back = [&]() {
        if (to_append.length() == 0) return;
        if (parts.size() > 0) throw runtime_error("Should not be unhandled");
        if (ModuleRegistry::Has(to_append) || this->LoadedModules.contains(to_append)) {
            possible_package_method = true;
            parts.push_back(TokenItem(TokenItem::TokenItem::PackageName, to_append));
        }
        else parts.push_back(TokenItem(TokenItem::TokenItem::Value, to_append));
        to_append = "";
    };
    string operator_check = "";
    for (int i = 0; i < value.length(); i++) {
        char c = value[i];
        
        
        if (depth.Get() == 0) {
            // Ensure no top-level operators exist, which would automatically fail
            // this check as it is an arithmetic, not stack call
            if (Utils::Find(LanArithmetic::OperatorCharacters(), c)) {
                string op = "";
                for (int j = 0; j < value.length() - i; j++) {
                    char k = value[i + j];
                    if (!Utils::Find(LanArithmetic::OperatorCharacters(), k)) break;
                    op += k;
                }
                if (LanArithmetic::IsOperator(op)) return {};
            }

            // If a space is found, it could be a new statement, which should automatically
            // fail this so that it gets forced
			if (c == ' ' && to_append == "new") {
                return {};
			}
            // Check to see if it could be a bracket index
            // The only way to disqualify this is if it contains commas,
            // which indicates an array.
            if (c == '[') {
                place_back();
                DepthEngine d = DepthEngine();
                bool valid_extention = true;
                string s = "";
                int j = 0;
                for (j = 1; j < value.length() - i; j++) {
                    char k = value[i + j];
                    if (d.Get() == 0 && k == ',') valid_extention = false; // Comma at top-level found, which indicates an array
                    d.Place(k);
                    if (d.depth['['] == -1) break; // The closing bracket that matches the first was found, exiting check
                    s += k;
                }
                i += j; // Adjust the global runner to skip over the already-analyzed part
                if (valid_extention && parts.size() > 0) {
                    // It looks like a valid extention, and it is not the first array-like thing.
                    parts.push_back(TokenItem(TokenItem::TokenItem::BracketExtention, s));
                }
                else if (parts.size() == 0) {
                    // Not a valid extention (is array), but being first, it can be allowed
                    std::string full_array = "[" + s + "]";
                    parts.push_back(TokenItem(TokenItem::TokenItem::Value, full_array));
                }
                //else if (valid_extention && parts.size() == 0 && s.empty()) {
                //    // Empty array
                //    std::string full_array = "[]";
                //    parts.push_back(TokenItem(TokenItem::TokenItem::Value, full_array));
                //}
                else throw runtime_error("Odd array call found: " + std::format("{}", valid_extention) + " " + std::format("{}", parts.size()) + " '" + s + "'");
            }
            // Check for colon extention
            else if (c == ':') {
                place_back();
                string s = "";
                int j = 0;
                for (j = 1; j < value.length() - i; j++) {
                    char k = value[i + j];
                    if (!std::isalnum(k)) break;
                    s += k;
                }
                i += j - 1; // Adjust the global runner to skip over the already-analyzed part
                parts.push_back(TokenItem(TokenItem::TokenItem::ColonExtention, s));
            }
            // Check for method
            else if (c == '.') {
                place_back();
                DepthEngine d = DepthEngine();
                string s = "";
                int j = 0;
                for (j = 1; j < value.length() - i; j++) {
                    char k = value[i + j];
                    d.Place(k);
                    s += k;
                    if (k == ')' && d.depth['('] == 0) break;
                }
                i += j; // Adjust the global runner to skip over the already-analyzed part
                if (possible_package_method) {
                    parts.push_back(TokenItem(TokenItem::TokenItem::PackageMethod, s));
                    possible_package_method = false;
                }
                else parts.push_back(TokenItem(TokenItem::TokenItem::Method, s));
            }
            else {
                depth.Place(c);
                to_append += c;
            }
        }
        else {
            depth.Place(c);
            to_append += c;
        }
    }

    if (parts.size() == 0 && !to_append.empty()) place_back();

    for (auto& p : parts) {
        CommandLineInterface::DebugPrint("Part '" + p.value + "', type " + p.type_string(), 1);
    }
    return parts;
}


bool MylangeInterpreter::ParseParameter(const string& rawParamStr, std::shared_ptr<LanVariable>& var, bool assignVar)
{
    auto res = this->ParseParameter(rawParamStr);
    if (res.has_value()) {
        if (assignVar) *var = res.value();
        return true;
    }
    else return false;
}

LanVariable MylangeInterpreter::ForcedParseParameter(const string& rawParamStr) {
    auto res = this->ParseParameter(rawParamStr);
    if (res.has_value()) {
        return res.value();
    }
    else throw runtime_error("Critical Parse failed on: " + rawParamStr);
}

optional<LanVariable> MylangeInterpreter::ParseParameter(const string& rawParamStr)
{
    string paramStr = Utils::TrimString(rawParamStr);
    CommandLineInterface::DebugPrint("Parsing parameter: " + paramStr);

    auto tokens = TokenizeComplexValue(paramStr);

    // Split into two cases,
    // A single token (simple type, cast right away
    // Multi tokens, means that the stack needs to be analyzed

    if (tokens.size() <= 1) {
        // A single value, randomtype or variable reference
        shared_ptr<LanVariable> result;
        smatch match;
        // Cache reference
        if (regex_match(paramStr, match, cachedBit))
        {
            CommandLineInterface::DebugPrint("Found cached something.");
            if (match[1].str() == "0") return move(this->InterpretBlock(match[0].str()));
            else if (match[1].str() == "1")
                return LanVariable::String(this->BlockMap[match[0].str()]);
            else if (match[1].str() == "2")
                return LanVariable::Char(this->BlockMap[match[0].str()].at(0));
            else if (match[1].str() == "3")
                return this->ParseParameter('{' + this->BlockMap[match[0].str()] + '}');
			else throw runtime_error("Unknown cached block type: " + match[1].str());
        }
        // Possible variable reference (soley word chars)
        else if (regex_match(paramStr, match, wordCharsOnly) && !Utils::Find(protectedWords, paramStr))
        {
            CommandLineInterface::DebugPrint("Possible variable found: " + paramStr, 1);
            if (this->Memory.resolve(paramStr, result)) {
                CommandLineInterface::DebugPrint("Found variable: " + paramStr, 1);
                return *result;
            }
            else throw runtime_error("Variable not found: " + paramStr);
        }
        // Type
        else if (regex_match(paramStr, match, regex(R"(^\$([\w<>| ]+)$)"))) {
            CommandLineInterface::DebugPrint("Type found: " + paramStr, 1);
            auto t = LanType::FromString(match[1].str());
            return LanVariable(LanType(LanTypeEnum::TypeType), t);
        }
        // Failsafe Random Type Conversion
        else if (this->RandomTypeConversion(paramStr, result))
        {
            return *result;
        }
        // Arithmetic Statement
        else if (LanArithmetic::IsValidLogicString(paramStr)) {
            CommandLineInterface::DebugPrint("Found Arithmetic Statement: " + paramStr);
            return LanArithmetic::BuildAST(paramStr)->Evaluate(*this);
        }
        // Possible function call
        else if (regex_search(paramStr, match, functionCallStack)) {
            CommandLineInterface::DebugPrint("Found function call: " + paramStr);
            auto res = this->FindFunction(paramStr);
            return res.first->Execute(*this, res.second);
        }
    }
    else {
        std::optional<LanVariable> working;
        std::string pack_path;
        for (int i = 0; i < tokens.size(); i++) {
            auto& token = tokens[i];
            CommandLineInterface::DebugPrint("Working on token [" + to_string(i + 1) + "/" + to_string(tokens.size()) + "]: " + token.value + " | " + token.type_string());
            switch (token.type) {
            case TokenItem::Value:
                working = this->ForcedParseParameter(token.value);
                break;
            case TokenItem::ColonExtention:
                if (working.has_value())
                {
                    working = *working->Index(token.value);
                }
                else throw runtime_error("Someone is bad");
                break;
            case TokenItem::BracketExtention:
            {
                auto a = this->ParseParameter(token.value);
                if (a.has_value() && working.has_value()) {
                    if (working.value().Type.IsArrayType() || working.value().Type == LanTypeEnum::TypeString)
                        working = *working->Index(std::get<int>(a.value().Value));
                    else if (working.value().Type.IsSetType())
                        working = *working->Index(std::get<string>(a.value().Value));
                }
                else throw runtime_error("Someone is bad");
            }
                break;
            case TokenItem::PackageName:
                if (i > 0) throw runtime_error("Package name use should not be in stack middleman.");
                pack_path = token.value;
                break;
            case TokenItem::Method:
            {
                if (!working.has_value()) throw runtime_error("Calling method on nil value.");
                else {
                    if (working.value().Type == LanTypeEnum::TypeCasting) {
                        auto& self_casting = std::get<shared_ptr<LanCasting>>(working.value().Value);
                        auto y = this->GetFunctionParts(token.value);
                        return self_casting->RunMethod(*this, y.first, y.second);
                    }
                    else {
                        auto res = this->FindFunction(token.value, working.value().Type.ToPackageString(), { working.value() });
                        working = res.first->Execute(*this, res.second);
                    }
                }
            }
                break;
            case TokenItem::PackageMethod:
            {
                auto res = this->FindFunction(token.value, pack_path);
                working = res.first->Execute(*this, res.second);
            }
                break;
            }
        }
        if (working.has_value()) return working.value();
        else return nullopt;
    }
}

std::pair<std::string, std::vector<LanVariable>> MylangeInterpreter::GetFunctionParts(const string& functionCallStr) {
    string function_name = "";
    vector<LanVariable> function_parameters;
    vector<LanType> function_parameters_types;
    
    smatch match;
    if (regex_search(functionCallStr, match, functionPartsPattern)) {
        string param_str = match[2].str();
        this->MakeParameters(param_str, function_parameters, function_parameters_types);

        function_name = match[1].str();

        CommandLineInterface::DebugPrint("Function name: " + match[1].str(), 1);
        CommandLineInterface::DebugPrint("Function params: " + match[2].str(), 1);
    }

    string functionId = LanFunction::GetId(function_name, function_parameters_types);

    return { functionId, function_parameters };
}

pair<shared_ptr<LanFunction>, vector<LanVariable>> MylangeInterpreter::FindFunction(const string& functionCallStr, std::string packagePath, vector<LanVariable> self)
{
    string function_name = "";
    vector<LanVariable> function_parameters;
    vector<LanType> function_parameters_types;

    for (auto& self_var : self) {
        function_parameters.push_back(self_var);
        function_parameters_types.push_back(self_var.Type);
    }

    smatch match;
    if (regex_search(functionCallStr, match, functionPartsPattern)) {
        string param_str = match[2].str();
        this->MakeParameters(param_str, function_parameters, function_parameters_types);

        function_name = match[1].str();

        CommandLineInterface::DebugPrint("Function name: " + match[1].str(), 1);
        CommandLineInterface::DebugPrint("Function params: " + match[2].str(), 1);
    }

    string functionId = LanFunction::GetId(function_name, function_parameters_types);
    CommandLineInterface::DebugPrint("Looking for function with id: " + functionId, 1);

	auto function = this->FindFunction(function_name, function_parameters_types, packagePath);
	return { function, function_parameters };

}

shared_ptr<LanFunction> MylangeInterpreter::FindFunction(const string& name, std::vector<LanType> paramTypes, const std::string path, bool excludeAny)
{
    // Overload param vectors
    std::vector<LanType> complxed_any_vector = {};
    for (auto& p : paramTypes) {
        if (p.IsArrayType()) complxed_any_vector.push_back(LanType(LanTypeEnum::TypeArray | LanTypeEnum::TypeAny));
        else if (p.IsSetType()) complxed_any_vector.push_back(LanType(LanTypeEnum::TypeSet));
        else complxed_any_vector.push_back(p);
    }
    std::vector<LanType> full_any_vector = std::vector<LanType>(paramTypes.size(), LanType(LanTypeEnum::TypeAny));
    
	// Generate IDs to look for (normal, complex any, full any)
	auto ids = std::vector<std::string>{
        LanFunction::GetId(name, paramTypes),
		LanFunction::GetId(name, complxed_any_vector),
		LanFunction::GetId(name, full_any_vector)
	};
	// Remove the possible duplicates, as they are not needed
    auto [first, last] = std::ranges::unique(ids);
    ids.erase(first, last);
    
    // Look for a package and function
    if (path != "") {
        if (this->LoadedModules.find(path) != this->LoadedModules.end()) {
            // Find scope
            auto scope = this->Memory.FindSiblingScope(path);
            if (!scope) throw runtime_error("Package scope not found: " + path);

			for (auto& id : ids) {
				auto fv = scope->resolve(id);
				if (fv) 
                {
                    CommandLineInterface::DebugPrint(std::format("Overload found [{}]: {}", path, id), 2);
                    return std::get<std::shared_ptr<LanFunction>>(fv->Value);
                }
				else CommandLineInterface::DebugPrint(std::format("Overload not found [{}]: {}", path, id), 2);
			}

            throw runtime_error(std::format("Function not found in package [{}]: {}", path, Utils::JoinStrings(ids, "/")));
        }
        else throw runtime_error("Package not found: " + path);
    }
    else {
		for (auto& id : ids) {
			shared_ptr<LanVariable> resultContainer;
			if (this->Memory.resolve(id, resultContainer)) {
				CommandLineInterface::DebugPrint("Found function: " + id);
				return std::get<std::shared_ptr<LanFunction>>(resultContainer->Value);
			}
			else CommandLineInterface::DebugPrint("Function not found: " + id, 2);
		}

        throw runtime_error(std::format("Function not found: {}", Utils::JoinStrings(ids, "/")));
    }

    throw runtime_error(std::format("Function not specified: {}", Utils::JoinStrings(ids, "/")));
}
;

void MylangeInterpreter::MakeParameters(string& paramString,
    vector<LanVariable>& paramsOut, vector<LanType>& paramTypesOut)
{
    vector<string> param_strs = Utils::TopLevelSplit(paramString, ',');
    for (const auto& param_str : param_strs) {
        auto param = this->ParseParameter(Utils::TrimString(param_str));
        if (param.has_value()) {
            auto& p = param.value();
            paramTypesOut.push_back(p.Type);
            paramsOut.push_back(p);
        }
        else
        {
            throw runtime_error("Failed to parse function argument: " + param_str);
        }
    }
}

bool MylangeInterpreter::RandomTypeConversion(const string& value, std::shared_ptr<LanVariable>& var)
{
    optional<LanVariable> test = this->RandomTypeConversion(value);
    if (test.has_value())
    {
        var = make_shared<LanVariable>(test.value());
        return true;
    }
    return false;
}



optional<LanVariable> MylangeInterpreter::RandomTypeConversion(const string& value)
{
    string trimmedValue = Utils::TrimString(value);
    CommandLineInterface::DebugPrint(std::format("Attempting to convert value: '{}'", trimmedValue));
    smatch matchedMatch;
    // nil
    if (trimmedValue == "nil")
    {
        CommandLineInterface::DebugPrint("Found nil");
        return LanVariable(
            LanType(LanTypeEnum::TypeNil),
            LanVariable::LanValue{});
    }
    // bool
    else if (trimmedValue == "true" || trimmedValue == "false")
    {
        CommandLineInterface::DebugPrint("Found bool");
        return LanVariable(
            LanType(LanTypeEnum::TypeBool),
            LanVariable::LanValue{ trimmedValue == "true" }
        );
    }
    //float
    else if (regex_match(trimmedValue, regex(R"(^-?\d+\.\d+$)")))
    {
        CommandLineInterface::DebugPrint("Found float");
        // Placeholder implementation
        return LanVariable(
            LanType(LanTypeEnum::TypeFloat),
            LanVariable::LanValue{ }
        );
    }
    // int
    else if (regex_match(trimmedValue, regex(R"(^-?\d+$)")))
    {
        CommandLineInterface::DebugPrint("Found int");
        return LanVariable(
            LanType(LanTypeEnum::TypeInt),
            LanVariable::LanValue{ stoi(trimmedValue) }
        );
    }
    // char
    else if (regex_match(trimmedValue, regex(R"(^'.'$)")))
    {
        CommandLineInterface::DebugPrint("Found char");
        return LanVariable(
            LanType(LanTypeEnum::TypeChar),
            LanVariable::LanValue{ trimmedValue[1] }
        );
    }
    // string
    else if (regex_match(trimmedValue, regex(R"(^".*"$)")))
    {
        CommandLineInterface::DebugPrint("Found str");
        return LanVariable(
            LanType(LanTypeEnum::TypeString),
            LanVariable::LanValue{ trimmedValue.substr(1, trimmedValue.length() - 2) }
        );
    }
    // array
    else if (Utils::IsWrappedByParens(trimmedValue, '[', ']') && regex_match(trimmedValue, regex(R"(^\[(.*)\]$)")))
    {
        CommandLineInterface::DebugPrint("Found arr: " + trimmedValue);
        vector<string> elementStrings = Utils::TopLevelSplit(
            trimmedValue.substr(1, trimmedValue.length() - 2), ','
        );
        vector <std::shared_ptr< LanVariable >> elements;
        LanType type = LanType(LanTypeEnum::TypeArray);
        for (auto& elemStr : elementStrings) {
            string trimmedElemStr = Utils::TrimString(elemStr);
            CommandLineInterface::DebugPrint("Array element string: " + trimmedElemStr, 1);
            optional <LanVariable> elemVar = this->ParseParameter(trimmedElemStr);
            if (elemVar.has_value()) {
				type.AddArchetype(elemVar.value().Type);
                elements.push_back(make_shared<LanVariable>(elemVar.value()));
            }
            else {
                throw runtime_error("Failed to parse array element: " + trimmedElemStr);
            }
        }
        return LanVariable(
            type,
            LanVariable::LanValue{ elements }
        );
    }
    // set || table
    else if (
        (Utils::IsWrappedByParens(trimmedValue, '(', ')') && regex_match(trimmedValue, regex(R"(^\((?:\s*\w+\s*=>.*)*\))")))
        || (Utils::IsWrappedByParens(trimmedValue, '{', '}') && regex_match(trimmedValue, regex(R"(^\{(?:\s*\w+\s*=>.*)*\})")))
        ) {
        bool isSetOverTable = Utils::IsWrappedByParens(trimmedValue, '(', ')');


        CommandLineInterface::DebugPrint(std::format("Found {}: {}", isSetOverTable?"set":"table", trimmedValue));

        vector<string> elementStrings = Utils::TopLevelSplit(
            trimmedValue.substr(1, trimmedValue.length() - 2), ','
        );

        std::vector<std::string> keys = {};
		std::vector<std::shared_ptr<LanVariable>> values = {};

        for (auto& part : elementStrings) {
            vector<string> parts = Utils::TopLevelSplit(part, "=>");
            if (parts.size() != 2) throw runtime_error("Cannot have mulitple => in set/table.");
            string key = Utils::TrimString(parts[0]);
            auto value = this->ParseParameter(Utils::TrimString(parts[1]));
            if (value.has_value()) {
				keys.push_back(key);
				values.push_back(make_shared<LanVariable>(value.value()));
            }
            else throw runtime_error("Could not parse value for set/table: " + Utils::TrimString(parts[1]));
        }

        return isSetOverTable
            ? LanVariable::MakeDict<LanSet>(LanTypeEnum::TypeSet, keys, values)
            : LanVariable::MakeDict<LanTable>(LanTypeEnum::TypeTable, keys, values);

        
    }
    // casting
    else if (regex_match(trimmedValue, matchedMatch, castingCreationPattern)) {
        CommandLineInterface::DebugPrint("Found new casting: " + trimmedValue);
		string target_type_str = matchedMatch[1].str();
		string param_str = matchedMatch[2].str();

        vector<LanVariable> params;

		for (auto& p : Utils::TopLevelSplit(param_str, ',')) {
            auto param = this->ParseParameter(Utils::TrimString(p));
            if (param.has_value())
                params.push_back(move(param.value()));
            else throw runtime_error("Failed to parse casting parameter: " + p);
        }

        CommandLineInterface::DebugPrint(std::format("Params: {}", params.size()));

        LanType target_type = this->ResolveType(target_type_str);
        shared_ptr<LanCasting> casting = std::make_shared<LanCasting>(target_type.CustomClass);
		
        string func_id = LanFunction::GetId(target_type.CustomClass->Name, params);
        CommandLineInterface::DebugPrint(std::format("Params for '{}': {}", func_id, params.size()));

        casting->RunMethod(*this, func_id, params); // Call the constructor

        return LanVariable(
            target_type,
            LanVariable::LanValue{ casting }
		);
    }
    // Iterable
    else if (regex_match(trimmedValue, matchedMatch, LanIterableEngine::RegexMatch)) {
        CommandLineInterface::DebugPrint("Found iter: " + trimmedValue);
        bool unpacking_type = false;
        vector<pair<string, LanType>> keys;
        if (Utils::IsWrappedByParens(Utils::TrimString(matchedMatch[1]), '[', ']')) {
            CommandLineInterface::DebugPrint("Upacking type detected: '" + matchedMatch[1].str() + "'");
            unpacking_type = true;
            string bare_elements = Utils::TrimString(matchedMatch[1].str());
            bare_elements = bare_elements.substr(1, bare_elements.length() - 2);
            for (auto& bare_ele : Utils::TopLevelSplit(bare_elements, ',')) {
                smatch key_parts;
                regex_search(bare_ele, key_parts, paramStringPattern);
                CommandLineInterface::DebugPrint(key_parts[3].str() + " :: " + key_parts[2].str());
                keys.push_back({key_parts[3].str(), LanType::FromString(key_parts[2].str())});
            }
        }
        else {
            smatch key_parts;
            string s = Utils::TrimString(matchedMatch[1]);
            regex_match(s, key_parts, paramStringPattern);
            keys.push_back({ key_parts[3].str(), LanType::FromString(key_parts[2].str()) });
        }
        auto matrix_value = this->ParseParameter(matchedMatch[2].str());
        if (matrix_value.has_value()) {
            return LanVariable(
                LanType(LanTypeEnum::TypeInterable),
                LanVariable::LanValue{ make_unique<LanIterableEngine>(keys, move(matrix_value.value())) }
            );
        }
        else throw runtime_error("Tried to obtain a null value for Iterable."); 
    }
    // Lambda
    else if (regex_match(trimmedValue, matchedMatch, lambdaPattern)) {
		// 1: return, 2: params types, 3: body
        LanType return_type = this->ResolveType(matchedMatch[1]);
        LanFunction::ParamStruct params = {};

		for (auto& p : Utils::TopLevelSplit(matchedMatch[2].str(), ',')) {
			smatch key_parts;
			regex_search(p, key_parts, paramStringPattern);
			string name = key_parts[3].str();
			LanType type = this->ResolveType(key_parts[2].str());
			params.push_back({ name, type });
		}

        shared_ptr<ScriptFunction> lf = make_shared<ScriptFunction>(return_type, "Lambda", params, matchedMatch[3]);

		return LanVariable(
			LanType(LanTypeEnum::TypeFunction),
			LanVariable::LanValue{ lf }
		);

    }
    // Unknown
    else return nullopt;
}



static std::vector<std::string> splitDotParenAware(const std::string& input)
{
    std::vector<std::string> parts;
    std::string current;

    int parenDepth = 0;

    for (size_t i = 0; i < input.size(); ++i)
    {
        char c = input[i];

        if (c == '(')
        {
            ++parenDepth;
            current += c;
        }
        else if (c == ')')
        {
            --parenDepth;
            current += c;
        }
        else if (c == '.' && parenDepth == 0)
        {
            // Split point: dot at top level
            parts.push_back(current);
            current.clear();
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
        parts.push_back(current);

    return parts;
};

CodeBlock::CodeBlock(const string& myScopeId)
{
    this->MyScopeID = myScopeId;
};