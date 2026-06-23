// IMPORTS //
#include <vector>
#include <string>
#include <regex>
#include <functional>
#include <unordered_map>
#include <map>
#include <optional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <cstdint>
#include <exception>
#include <type_traits>
#include <variant>

#include "MemoryBooker.h"
#include "MylangeInterpreter.h"
#include "CommandLineInterface.h"
#include "LanType.h"
#include "LanArithmetic.h"
#include "Utils.h"
#include "LanVariable.h"
#include "LanIterableEngine.h"
#include "LanClass.h"
#include "ModuleRegistry.h"



using namespace std;

const regex functionPartsPattern(R"(^([\w.]+)\s*\((.*)\))", std::regex_constants::ECMAScript);
const regex singleFunctionPartPattern(R"(^(\w+)\s*\((.*)\)$)", std::regex_constants::ECMAScript);
const regex functionCallStack(R"(^(?:\w+\.?)+(?:\w+\(.*\))+)", std::regex_constants::ECMAScript);
const regex functionDotExtention(R"((\)\.))", std::regex_constants::ECMAScript);
const regex ifElseThenPattern(R"(^if\s*\((.*?)\)\s*then\s*(.*))", std::regex_constants::ECMAScript);
const regex paramStringPattern(R"((?:(const)\s+)?([\w<|,>]+)\s+(\w+))", std::regex_constants::ECMAScript);
const regex functionMethodDeclaration(R"(^((?:@?\w+\s+)+)?def\s+(\w+)\s+(\w+)\s*\((.*)\)\s*as\s*(.*))", std::regex_constants::ECMAScript);
const regex classDeclarationPatter(R"(^class\s+(\w+)\s+(?:extends\s+(\w+))?\s*has\s*(.*))", std::regex_constants::ECMAScript);
const regex wordCharsOnly(R"(^[a-zA-Z]\w+$)", std::regex_constants::ECMAScript);
const regex cachedBit(R"((\d)x([a-fA-F0-9]+))", std::regex_constants::ECMAScript);

// For Classes

const regex classDefualtPropertyPatter(R"(^((?:@?\w+\s+)+)?\s*([a-zA-Z<>,|\s]+) +(\w+) *=> *(.*))", std::regex_constants::ECMAScript);
const regex classPropertyPattern(R"(^((?:@?\w+\s+)+)?\s*([a-zA-Z<>,|\s]+) +(\w+))", std::regex_constants::ECMAScript);
const regex castingCreationPattern(R"(^new\s+([\w]+)\((.*)\))", std::regex_constants::ECMAScript);
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
    // Return
    {
        regex(R"(return\s+(.*))"),
        [](auto const& m, MylangeInterpreter& mi) {
            auto it = mi.ParseParameter(m[1].str());
            if (it.has_value()) 
                //return move(it.value());
                //return optional<LanVariable>(move(it.value()->Clone()));
				return optional<LanVariable>(it.value());
            throw runtime_error("Trying to return nothing");
        }
    },
    // Include
    {
        regex(R"(^#include\s*<(\w+)>)"),
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
    // Set Variable
    {
        regex(R"(^\s*([a-zA-Z<>,|\s]+) +(\w+) *=> *(.*))"),
        [](auto const& m, MylangeInterpreter& mi) {
            LanType expectedType;
            try {
                expectedType = LanType::FromString(Utils::TrimString(m[1]));
            }
            catch (exception& e) {
                throw runtime_error("Custom classes not implemented yet.");
				//unique_ptr<LanClass> customClass;
    //            if (mi.MemBook.GetClass(scopeId, Utils::TrimString(m[1]), customClass)) {
				//	expectedType = LanType(move(customClass));
    //            }
    //            else
				//throw runtime_error("Cannot resolve type: " + Utils::TrimString(m[1]) + " : " + e.what());
            }


            CommandLineInterface::DebugPrint("EXP TYPE:" + to_string(static_cast<uint32_t>(expectedType.BaseType)));
            auto lv = mi.ParseParameter(m[3]);
            if (!lv.has_value())
                throw runtime_error("Failed to parse variable value.");
            CommandLineInterface::DebugPrint("Setting variable " + m[2].str() + " of type " + expectedType.ToString()
                + "/" + lv.value().Type.ToString() + " to value " + lv.value().ToString());
            if (lv.value().IsCompatible(expectedType)) {
                //mi.MemBook.BookVariable(scopeId, m[2], move(lv.value()));
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
        regex(R"(^\s*(\w+) *=> *(.*))"),
        [](auto const& m, MylangeInterpreter& mi) { 
            string name = m[1].str();
            auto new_value = mi.ParseParameter(m[2].str());
            if (new_value.has_value()) {
                //mi.MemBook.RebookVariable(scopeId, name, move(new_value.value()));
                mi.Memory.assign(name, new_value.value());
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
                throw runtime_error("Class declaration not implemented" + extends);
                //unique_ptr<LanClass> parentClass;
                //if (mi.MemBook.GetClass(scopeId, extends, parentClass)) {
                //    CommandLineInterface::DebugPrint("Found parent class: " + extends);
                //    // Inherit properties
                //    for (const auto& [propName, propType] : parentClass->Properties) {
                //        properties[propName] = propType;
                //    }
                //    // Inherit default values
                //    for (const auto& [propName, defaultValue] : parentClass->DefaultValues) {
                //        defaultValues[propName] = defaultValue->Clone();
                //    }
                //    // Inherit methods
                //    for (const auto& [methodName, method] : parentClass->Methods) {
                //        methods[methodName] = method->Clone();
                //    }
                //}
                //else throw runtime_error("Parent class not found: " + extends);
			}

            for (auto& rawLine : lines) {
				string line = Utils::TrimString(rawLine);
				if (line.empty()) continue;
                CommandLineInterface::DebugPrint("Parsing class line: " + line);

                if (regex_match(line, match, classDefualtPropertyPatter)) {
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
                    //mi.MemBook.BookVariable(scopeId + "." + m[1].str(), property_name, make_unique<LanVariable>(property_type, LanVariable::LanValue{}));
					properties[property_name] = property_type;
                }
				else if (regex_match(line, match, functionMethodDeclaration)) {
                    CommandLineInterface::DebugPrint("Found method: " + match[3].str() + " of type " + match[2].str());
                    // Modifiers
                    vector<string> modifiers = Utils::TopLevelSplit(Utils::TrimString(match[1].str()), ' ');
                    // Method
                    string method_name = match[3].str();
                    LanType return_type = LanType::FromString(Utils::TrimString(match[2].str()));
                    map<string, LanType> parameter_map;
                    for (const auto& param_str : Utils::TopLevelSplit(match[4], ',')) {
                        auto parts = Utils::TopLevelSplit(Utils::TrimString(param_str), ' ');
                        if (parts.size() != 2) throw runtime_error("Each param must have 2 parts. Found " + to_string(parts.size()));
                        parameter_map[parts[1]] = LanType::FromString(parts[0]);
                    }
                    string logic = match[5].str();
                    unique_ptr<LanFunction> method =
                        make_unique<ScriptFunction>(return_type, method_name, parameter_map, logic);
                    // Check if override is needed
                    if ((methods.find(method->GetId()) != methods.end()) && !Utils::Find(modifiers, overrideString))
                        throw runtime_error("Cannot declare the same method without override.");
					methods[method->GetId()] = move(method);
                }
				else throw runtime_error("Invalid class body line: " + line);
            }

			//mi.MemBook.BookClass(scopeId, make_unique<LanClass>(name, properties, defaultValues, methods));
			mi.Memory.define(name, LanVariable(LanType(LanTypeEnum::TypeClass), make_shared<LanClass>(name, properties, defaultValues, methods)));

            return nullopt;
        }
    },
    // Functions
    {
        functionMethodDeclaration,
        [](auto const& m, MylangeInterpreter& mi) {
            /*cout << "RType:" << m[1] << endl << "Name:" << m[2] << endl
                << "ParamStr:" << m[3] << endl << "Logic:" << m[4] << endl;*/
            // Settup params
            map<string, LanType> parameter_map;
            for (const auto& param_str : Utils::TopLevelSplit(m[4], ','))
            {
                auto parts = Utils::TopLevelSplit(Utils::TrimString(param_str), ' ');
                if (parts.size() != 2) throw runtime_error("Each param must have 2 parts. Found " + to_string(parts.size()));
                parameter_map[parts[1]] = LanType::FromString(parts[0]);
            }
            const LanType return_type = LanType::FromString(m[2]);
            std::shared_ptr<LanFunction> function =
                make_unique<ScriptFunction>(return_type, m[3].str(), parameter_map, m[5].str());
			CommandLineInterface::DebugPrint("[" + mi.Memory.currentScope()->id + "] Registering function : " + function->GetId() + " with return type " + return_type.ToString());
            
			mi.Memory.define(function->GetId(), LanVariable(LanType(LanTypeEnum::TypeFunction), function));

			//mi.RegisteredFunctions->addFunction(scopeId, move(function));
			//mi.RegisteredFunctions->debugPrint();
            
            //mi.MemBook.BookFunction(scopeId, move(function));
            return nullopt;
        }
    },
    // Functional Execution
    {
        functionCallStack,
        [](auto const& m, MylangeInterpreter& mi) {
			mi.RunFunctionStack(m[0].str());
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
                    auto out = move(mi.InterpretBlock(part));
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
                            //mi.MemBook.BookVariable(loop_id, ie->Keys[i].first, move(value));
							mi.Memory.define(ie->Keys[i].first, *value);
                        }
                    }
                    else if constexpr (is_same_v<T, LanVariable>) {
                        auto& value = get<LanVariable>(valueVariant);
                        //CommandLineInterface::DebugPrint("Running loop with '" + ie->Keys[0].first + "' set as: " + value->ToString());
                        if (!value.IsCompatible(ie->Keys[0].second)) throw runtime_error("Type expected and given mismatch.");
                        //mi.MemBook.BookVariable(loop_id, ie->Keys[0].first, move(value));
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
                if (!std::get<bool>(condition_var.value().Value)) running = false;
                

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
    }
};

// CODE //
MylangeInterpreter::MylangeInterpreter()
{
    this->BlockCounter = 1;
	//this->RegisteredFunctions = make_unique<MasterFunctionTree>();
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
            std::string code = Utils::MakeHexCode("0x", counter++);
            (*map)[code] = inner;

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

    //std::cout << "Result:\n" << condensed_block << "\n\n";
    //std::cout << "Map contents:\n";
    /*for (const auto& [k, v] : this->BlockMap) {
        CommandLineInterface::DebugPrint(k + " -> [" + v + "]");
    }*/

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
    //if (!SkipClearing)
    //    this->Memory.popScope();
    return nullopt;
}



const std::vector<std::string> protectedWords = {
    "true", "false"
};

const regex fullVariablePattern(R"(([a-zA-Z]\w+)((?:(?::\w+)|(?:\[.+?\]))*))", std::regex_constants::ECMAScript);
const regex variableExtentionPattern(R"((?::\w+)|(?:\[.+?\]))", std::regex_constants::ECMAScript);


optional<LanVariable> MylangeInterpreter::ParseParameter(const string& rawParamStr)
{
	string paramStr = Utils::TrimString(rawParamStr);
	CommandLineInterface::DebugPrint("Parsing parameter: " + paramStr);
    shared_ptr<LanVariable> result;
    smatch match;
    // Cache reference
    if (regex_match(paramStr, match, cachedBit))
    {
        CommandLineInterface::DebugPrint("Found cached something.");
        if (match[1].str() == "0") return move(this->InterpretBlock(match[0].str()));
        else if (match[1].str() == "1")
            return LanVariable(
                LanType(LanTypeEnum::TypeString), 
                LanVariable::LanValue{ this->BlockMap[match[0].str()] }
            );
        else if (match[1].str() == "2")
            return LanVariable(
                LanType(LanTypeEnum::TypeChar),
                LanVariable::LanValue{ this->BlockMap[match[0].str()].at(0) }
            );
    }
    // Indexed variable
    else if (regex_match(paramStr, match, fullVariablePattern) && !Utils::Find(protectedWords, paramStr))
    {
		string baseName = match[1].str();
		string extentions = match[2].str();

        shared_ptr<LanVariable> baseVar;
        if (this->Memory.resolve(baseName, baseVar)) {
            std::sregex_iterator begin(extentions.begin(), extentions.end(), variableExtentionPattern);
            std::sregex_iterator end;
            for (std::sregex_iterator i = begin; i != end; ++i) {
                std::smatch match = *i;
				string matchStr = match.str();
                // Colon extention
                if (matchStr[0] == ':') {
					string indexStr = matchStr.substr(1);
                    baseVar = baseVar->Index(indexStr);
                }
                // Bracket extention
                else if (matchStr[0] == '[') {
                    string indexStr = matchStr.substr(1, matchStr.length() - 2);
                    auto indexVarOpt = this->ParseParameter(indexStr);
                    if (!indexVarOpt.has_value()) throw runtime_error("Failed to parse index in variable extention.");

                    if (indexVarOpt.value().IsCompatible(LanType(LanTypeEnum::TypeInt))) {
                        baseVar = baseVar->Index(std::get<int>(indexVarOpt.value().Value));
                    } else if (indexVarOpt.value().IsCompatible(LanType(LanTypeEnum::TypeString))) {
                        baseVar = baseVar->Index(std::get<string>(indexVarOpt.value().Value));
					} else throw runtime_error("Index in variable extention must be int or str. Got " + indexVarOpt.value().Type.ToString());
				}
            }
			CommandLineInterface::DebugPrint("Found variable with extentions: " + baseName + " with extentions: " + extentions
                + " with value" + baseVar->ToString(), 1);
            return std::optional<LanVariable>(*baseVar);
        }
		else throw runtime_error("Base variable not found: " + baseName);
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
        auto res = this->RunFunctionStack(paramStr);
        if (res) return std::move(res);
        return nullopt;
    }

    return nullopt;
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
    CommandLineInterface::DebugPrint("Attempting to convert value: " + value);
    string trimmedValue = Utils::TrimString(value);
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
    // int
    else if (regex_match(trimmedValue, regex(R"(^-?\d+$)")))
    {
        CommandLineInterface::DebugPrint("Found int");
        return LanVariable(
            LanType(LanTypeEnum::TypeInt),
            LanVariable::LanValue{ stoi(trimmedValue) }
        );
    }
    //float
    else if (regex_match(trimmedValue, regex(R"(^-?\d+\.\d+$)")))
    {
        CommandLineInterface::DebugPrint("Found float");
        // Placeholder implementation
        return LanVariable(
            LanType(LanTypeEnum::TypeUnknown),
            LanVariable::LanValue{ }
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
    else if (regex_match(trimmedValue, regex(R"(^\[(.*)\]$)")))
    {
        CommandLineInterface::DebugPrint("Found arr: " + trimmedValue);
        vector<string> elementStrings = Utils::TopLevelSplit(
            trimmedValue.substr(1, trimmedValue.length() - 2), ','
        );
        vector <std::shared_ptr< LanVariable >> elements;
        for (auto& elemStr : elementStrings) {
            string trimmedElemStr = Utils::TrimString(elemStr);
            CommandLineInterface::DebugPrint("Array element string: " + trimmedElemStr, 1);
            optional <LanVariable> elemVar = this->ParseParameter(trimmedElemStr);
            if (elemVar.has_value()) {
                elements.push_back(make_shared<LanVariable>(elemVar.value()));
            }
            else {
                throw runtime_error("Failed to parse array element: " + trimmedElemStr);
            }
        }
        return LanVariable(
            LanType(LanTypeEnum::TypeArray),
            LanVariable::LanValue{ elements }
        );
    }
    // set
    else if (regex_match(trimmedValue, regex(R"(^\((?:\s*\w+\s*=>.*)\))"))) {
        CommandLineInterface::DebugPrint("Found set: " + trimmedValue);
        vector<string> elementStrings = Utils::TopLevelSplit(
            trimmedValue.substr(1, trimmedValue.length() - 2), ','
        );
        unordered_map<string, shared_ptr<LanVariable>> set_map;
        for (auto& part : elementStrings) {
            vector<string> parts = Utils::TopLevelSplit(part, "=>");
            if (parts.size() != 2) throw runtime_error("Cannot have mulitple => in set.");
            string name = Utils::TrimString(parts[0]);
            auto value = this->ParseParameter(Utils::TrimString(parts[1]));
            if (value.has_value())
                set_map.emplace(name, make_shared<LanVariable>(value.value()));
            else throw runtime_error("Could not parse value for set: " + Utils::TrimString(parts[1]));
        }

        return LanVariable(
            LanType(LanTypeEnum::TypeSet),
            LanVariable::LanValue{ set_map }
        );
    }   
    // casting
    else if (regex_match(trimmedValue, matchedMatch, castingCreationPattern)) {
		string target_type_str = matchedMatch[1].str();
		string param_str = matchedMatch[2].str();

        vector<LanVariable> params;

		for (auto& p : Utils::TopLevelSplit(param_str, ',')) {
            auto param = this->ParseParameter(Utils::TrimString(p));
            if (param.has_value())
                params.push_back(move(param.value()));
            else throw runtime_error("Failed to parse casting parameter: " + p);
        }

        shared_ptr<LanVariable> customClassContainer;
        if (!this->Memory.resolve(target_type_str, customClassContainer))
			throw runtime_error("Cannot find type for casting: " + target_type_str);

        LanType target_type = LanType(std::get<std::shared_ptr<LanClass>>(customClassContainer->Value));
        shared_ptr<LanCasting> casting = std::make_shared<LanCasting>(std::get<std::shared_ptr<LanClass>>(customClassContainer->Value));
		
        casting->RunMethod(*this, target_type_str, params); // Call the constructor

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
    // unknown
    else return nullopt;
};

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
}


static void MakeParameters(MylangeInterpreter& mi, string paramString,
    vector<LanVariable>& paramsOut, vector<LanType>& paramTypesOut)
{
    vector<string> param_strs = Utils::TopLevelSplit(paramString, ',');
    for (const auto& param_str : param_strs) {
        auto param = mi.ParseParameter(Utils::TrimString(param_str));
        if (param.has_value()) {
            auto& p = param.value();
            paramsOut.push_back(move(p));
        }
        else
        {
            throw runtime_error("Failed to parse function argument: " + param_str);
        }
    }
    for (const auto& param : paramsOut) paramTypesOut.push_back(param.Type);
}

optional<LanVariable> MylangeInterpreter::RunFunctionStack(const string& functionStackStr)
{
    CommandLineInterface::DebugPrint("Executing function stack [" + this->Memory.currentScope()->id + "]: " + functionStackStr);

    auto function_calls = splitDotParenAware(functionStackStr);
    bool consiteringPackage = true;
    string packagePath = "";
	vector<string> hangingPath = vector<string>();
    for (auto& fc : function_calls) {
        CommandLineInterface::DebugPrint("Function call part: " + fc, 2);
        if (Utils::IsAlphanumeric(fc) && consiteringPackage) {
			if (packagePath.empty()) packagePath = fc;
			else packagePath += "." + fc;
        }
        else {
            if (consiteringPackage) consiteringPackage = false;
            hangingPath.push_back(fc);
        }
    }

	CommandLineInterface::DebugPrint("Package path: " + packagePath, 1);
	CommandLineInterface::DebugPrint("First function call: " + hangingPath[0], 1);
	CommandLineInterface::DebugPrint("Hanging path: " + Utils::JoinStrings(hangingPath, ", "), 1);

    // Setup initial value

    string init_funct_name = "";
    vector<LanVariable> init_funct_params;
    vector<LanType> init_funct_param_types;

    smatch match;
    if (regex_search(hangingPath[0], match, functionPartsPattern)) {
        
        MakeParameters(*this, match[2].str(), init_funct_params, init_funct_param_types);

        init_funct_name = match[1].str();

        CommandLineInterface::DebugPrint("Function name: " + match[1].str(), 1);
        CommandLineInterface::DebugPrint("Function params: " + match[2].str(), 1);
    }

	string functionId = LanFunction::GetId(init_funct_name, init_funct_param_types);
	CommandLineInterface::DebugPrint("Looking for function with id: " + functionId, 1);
    this->Memory.dump();

    optional<std::shared_ptr<LanVariable>> last_result = std::make_shared<LanVariable>();


    
    if (packagePath != "") {
		if (this->LoadedModules.find(packagePath) != this->LoadedModules.end()) {
            // Find scope
			auto scope = this->Memory.FindSiblingScope(packagePath);
			if (!scope) throw runtime_error("Package scope not found: " + packagePath);
			auto fv = scope->resolve(functionId);
            if (!fv) {
                // Try to find an overload with any types

				auto any_vector = vector<LanType>(init_funct_param_types.size(), LanType(LanTypeEnum::TypeAny));
                string any_functionId = LanFunction::GetId(init_funct_name, any_vector);
                
				fv = scope->resolve(any_functionId);
				if (!fv) throw runtime_error("Function not found in package: " + functionId);
            }
            
            auto& f = std::get<std::shared_ptr<LanFunction>>(fv->Value);
			auto a = f->Execute(*this, init_funct_params);
            if (a.has_value()) {
                last_result = std::make_shared<LanVariable>(a.value());
                CommandLineInterface::DebugPrint("Found function in package: " + functionId + " with value " + a.value().ToString());
            }
			//last_result = std::make_shared<LanVariable>(a->Type, a->Value);
		}
		else throw runtime_error("Package not found: " + packagePath);
    }
	else if (this->Memory.resolve(functionId, last_result.value())) {
		CommandLineInterface::DebugPrint("Found function: " + functionId);
        auto a = std::get<std::shared_ptr<LanFunction>>(last_result.value()->Value)->Execute(*this, init_funct_params);
	}
	else {
		throw runtime_error("Function not found: " + functionId);
	}

	return std::optional<LanVariable>(*last_result.value());
    
    /*
    if (auto l = this->RegisteredFunctions->findFunction(packagePath, functionId)) {
        // Packaged function
		CommandLineInterface::DebugPrint("Found packaged function: " + functionId + " in package " + packagePath);
        last_result = l->Execute(scopeId, *this, init_funct_params);
    }
    if (auto l = this->RegisteredFunctions->findFunction(scopeId, functionId)) {
        // Local function
		CommandLineInterface::DebugPrint("Found local function: " + functionId + " in scope " + scopeId);
        last_result = l->Execute(scopeId, *this, init_funct_params);
    }
    else {
        // Variable or other type
		last_result = this->ParseParameter(scopeId, hangingPath[0]);
    }




    return nullopt;
    // First, try seeing if it's a package reference
    // Then, using the scope to see if it's a user-function or variable reference

    auto l = this->RegisteredFunctions->findFunction(packagePath, "");

	auto last_result = make_unique<LanVariable>();

    for (int i = 0; i < hangingPath.size(); i++) {

    }
    */
	

    return nullopt;
}




/*
optional<LanVariable> MylangeInterpreter::RunFunctionStackOld(const string& functionStackStr)
{
    CommandLineInterface::DebugPrint("Executing function call(s): " + functionStackStr);
    auto function_calls = splitDotParenAware(functionStackStr);
	for (auto& fc : function_calls) {
        CommandLineInterface::DebugPrint("Function call part: " + fc, 1);
    }
    optional<LanVariable> last_result = make_unique<LanVariable>();
    string prefix = "";

    for (const auto& func_call : function_calls) {
		string function_location_scope_id =  (prefix.empty()) ? scopeId : prefix;
        //string scopeId = scopeIdRaw;
        smatch func_match;
        if (regex_search(func_call, func_match, functionPartsPattern)) {
            // find function
            string func_name = func_match[1];
            vector<LanVariable> params;
            vector<LanType> param_types;
			MakeParameters(*this, scopeId, func_match[2], params, param_types);
			// If the previoud result is a Casting, then try to get the function from the casting's custom class
            if (last_result.has_value() && (last_result.value()->Type.BaseType & LanTypeEnum::TypeCasting) == LanTypeEnum::TypeCasting) {
                
				unique_ptr<LanCasting>& casting = get<unique_ptr<LanCasting>>(last_result.value()->Value);

				auto v = casting->RunMethod(*this, scopeId, func_name, params);
				last_result = move(v);
            }
            // Type Literal approch first.
            else if (LanFunction* func = this->MemBook.GetFunction(function_location_scope_id, func_name, param_types))
            {
                CommandLineInterface::DebugPrint("Function " + func_name + " exists and is running...");
                last_result = move((func)->Execute(scopeId, *this, params));
            }
            // Then, try to see if there is an overload with the "any" type in unmatched positions.
            else {
                vector<LanFunction*> overloads = this->MemBook.GetFunctionOverloads(function_location_scope_id, func_name);
				CommandLineInterface::DebugPrint("Function " + func_name + " has " + to_string(overloads.size()) + " overloads.");
                bool found = false;
                for (auto& overload : overloads) {
                    const auto& param_map = overload->Parameters;
                    if (param_map.size() != params.size()) continue;
                    bool match = true;
                    auto param_it = param_map.begin();
                    for (size_t i = 0; i < params.size(); ++i, ++param_it) {
                        if (!(params[i]->Type == param_it->second ||
                            param_it->second.BaseType == LanTypeEnum::TypeAny)) {
                            match = false;
                            break;
                        }
                    }
                    if (match) {
                        CommandLineInterface::DebugPrint("Function " + func_name + " overload exists and is running...");
                        last_result = move(overload->Execute(scopeId, *this, params));
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    string err = "Function not found: " + func_name + " with parameter types: ";
				}
			}
        }
		// Check first to see if it's a variable reference to get the value
        else if (this->MemBook.GetVariable(scopeId, func_call, last_result.value()))
        {
			CommandLineInterface::DebugPrint("Variable reference found in function stack: " + func_call);
		}
        // Check then to see if it's a imported package name
        else if (this->RegisteredFunctions->findPackage(func_call))
        {
			CommandLineInterface::DebugPrint("Package reference found in function stack: " + func_call);
			prefix += func_call;
        }
		// Finally, throw error
        else throw runtime_error("Invalid function call syntax: " + func_call);
    }
    return last_result;
}
*/

CodeBlock::CodeBlock(const string& myScopeId)
{
	this->MyScopeID = myScopeId;
}
