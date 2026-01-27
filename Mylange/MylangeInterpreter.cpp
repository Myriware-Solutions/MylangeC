// IMPORTS //
#include <vector>
#include <string>
#include <regex>
#include <functional>
#include <unordered_map>
#include <map>
#include <optional>

#include "MemoryBooker.h"
#include "MylangeInterpreter.h"
#include "CommandLineInterface.h"
#include "IncludableFunctions.h"
#include "LanType.h"
#include "LanFunction.h"
#include "LanArithmetic.h"
#include "Utils.h"
#include "LanPackages.h"
#include <memory>
#include <stdexcept>
#include <utility>
#include "LanVariable.h"
#include "LanIterableEngine.h"


using namespace std;

const regex functionPartsPattern(R"(^([\w.]+)\s*\((.*)\))", std::regex_constants::ECMAScript);
const regex functionCallStack(R"(^(?:\w+\.?)+(?:\w+\(.*\))+)", std::regex_constants::ECMAScript);
const regex functionDotExtention(R"((\)\.))", std::regex_constants::ECMAScript);
const regex ifElseThenPattern(R"(^if\s*\((.*?)\)\s*then\s*(.*))", std::regex_constants::ECMAScript);
const regex paramStringPattern(R"((?:(const)\s+)?([\w<|,>]+)\s+(\w+))", std::regex_constants::ECMAScript);

struct Rule {
    regex pattern;
    function <optional <unique_ptr< LanVariable >> (const smatch&, MylangeInterpreter&, const string&) > action;
};

vector<Rule> rules = {
    // Return
    {
        regex(R"(return\s+(.*))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
            auto it = mi.ParseParameter(scopeId, m[1].str());
            if (it.has_value()) return move(it.value());
            throw runtime_error("Trying to return nothing");
        }
    },
    // Include
    {
        regex(R"(^#include\s*<(\w+)>)"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
			string package_name = m[1].str();
			CommandLineInterface::DebugPrint("Including package: " + package_name);

            unique_ptr<vector<string>> package_functions;
            if (LanPackages::HasPackage(package_name, package_functions)) {
				mi.ImportedPackages.push_back(package_name);
                for (auto& func_name : *package_functions) {
                    unique_ptr<LanFunction> func;
                    if (IncludableFunctions::GetIncludableFunction(func_name, func))
                    {
                        CommandLineInterface::DebugPrint("Included package function: " + func_name + " : " + (func)->GetId(), 1);
						//string func_scope_id = scopeId + "." + package_name;
                        mi.MemBook.BookFunction(package_name, std::move(func));
                    }
                    else throw runtime_error("Package Function not found: " + func_name);
                }
            }
            else throw runtime_error("Package not found: " + package_name);
            return nullopt;
        }
    },
    // Set Variable
    {
        regex(R"(^\s*([a-zA-Z<>,|\s]+) +(\w+) *=> *(.*))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
			LanType expectedType = LanType::FromString(Utils::TrimString(m[1]));
            CommandLineInterface::DebugPrint("EXP TYPE:" + to_string(static_cast<uint32_t>(expectedType.BaseType)));
            //auto lv = LanVariable::RandomTypeConversion(m[3]);
			auto lv = mi.ParseParameter(scopeId, m[3]);
            if (!lv.has_value()) 
                throw runtime_error("Failed to parse variable value.");
			CommandLineInterface::DebugPrint("Setting variable " + m[2].str() + " of type " + expectedType.ToString() + "/" + lv.value()->Type.ToString() + " to value " + lv.value()->ToString());
            if (lv.value()->IsCompatable(expectedType))
                mi.MemBook.BookVariable(scopeId, m[2], move(lv.value()));
            else throw runtime_error("Type mismatch in variable assignment. Expected " 
                + expectedType.ToString() + ", got " + lv.value()->Type.ToString() 
                + " (with " + lv.value()->ToString() + ")");
            return nullopt;
        }
    },
    // Reset variable
    {
        regex(R"(^\s*(\w+) *=> *(.*))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) { 
            string name = m[1].str();
            auto new_value = mi.ParseParameter(scopeId, m[2].str());
            if (new_value.has_value())
                mi.MemBook.RebookVariable(scopeId, name, move(new_value.value()));
            else throw runtime_error("Missing value in reset.");
            return nullopt;
        }
    },
    // Do Block
    {
        regex(R"(^do\s+(.*))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
			CommandLineInterface::DebugPrint("[" + scopeId + "] Interpreting block: " + m[1].str());
			mi.InterpretBlock(scopeId + ".do", m[1]);
            return nullopt;
        }

    },
    // Cached Block
    {
        regex(R"(^(0x[a-f0-9]+))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
            CommandLineInterface::DebugPrint("Found cached block: " + m[1].str());
            string cached_block = mi.BlockMap[m[1]];
            mi.InterpretBlock(scopeId + "." + m[1].str(), cached_block);
            return nullopt;
        }
    },
    // Functions
    {
        regex(R"(^def\s+(\w+)\s+(\w+)\s*\((.*)\)\s*as\s*(.*))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
            /*cout << "RType:" << m[1] << endl << "Name:" << m[2] << endl
                << "ParamStr:" << m[3] << endl << "Logic:" << m[4] << endl;*/
            // Settup params
            map<string, LanType> parameter_map;
            for (const auto& param_str : Utils::TopLevelSplit(m[3], ','))
            {
                auto parts = Utils::TopLevelSplit(Utils::TrimString(param_str), ' ');
                if (parts.size() != 2) throw runtime_error("Each param must have 2 parts. Found " + to_string(parts.size()));
                parameter_map[parts[1]] = LanType::FromString(parts[0]);
            }
            const LanType return_type = LanType::FromString(m[1]);
            //unique_ptr<LanFunction> function = make_unique<LanFunction>(return_type, m[2].str(), parameter_map, m[4].str());
            unique_ptr<LanFunction> function =
                make_unique<ScriptFunction>(return_type, m[2].str(), parameter_map, m[4].str());
            mi.MemBook.BookFunction(scopeId, move(function));
            return nullopt;
        }
    },
    // Functional Execution
    {
        functionCallStack,
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
			mi.RunFunctionStack(scopeId, m[0].str());
            return nullopt;
        }
    },
    // If/Else/Then Block
    {
        ifElseThenPattern,
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
			auto parts = Utils::SplitString(m[2], "else");
			for (auto& part : parts) {
                part = Utils::TrimString(part);
                smatch match;
                if (regex_match(part, match, ifElseThenPattern))
                {
					auto conditionEval = mi.ParseParameter(scopeId, m[1].str());

                    if (conditionEval.has_value() && ((conditionEval.value()->Type.BaseType & LanTypeEnum::TypeBool) == LanTypeEnum::TypeBool)
                        && get<bool>(conditionEval.value()->Value))
                    {
						mi.InterpretBlock(scopeId + ".if", match[2].str());
                        return nullopt;
                    }
                }
                else {
					mi.InterpretBlock(scopeId + ".if", part);
                    return nullopt;
                }
            }
            return nullopt;
        }
    },
    // For loop
    {
        regex(R"(for\s*\((.*)\)\s*do\s*(.*))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
			CommandLineInterface::DebugPrint("Interpreting for loop: " + m[0].str());
            // Get the iter variable to loop over
            auto iter = mi.ParseParameter(scopeId, m[1].str());
            string loop_id = scopeId + ".for";
            if (!iter.has_value()) throw runtime_error("Cannot use undefined value for looping.");
            auto& ie = get<unique_ptr<LanIterableEngine>>(iter.value()->Value);
            CommandLineInterface::DebugPrint("This iter has " + to_string(ie->Values.size()) + " value");
            for (auto& valueVariant : ie->Values) {
                // Create the parameters as variables inside the loop
                visit([&](const auto& value) {
                    using T = decay_t<decltype(value)>;
                    if constexpr (is_same_v<T, vector<unique_ptr<LanVariable>>>) {
                        auto& value_vector = get<vector<unique_ptr<LanVariable>>>(valueVariant);
                        for (int i = 0; i < value_vector.size(); i++) {
                            auto& value = value_vector[i];
                            if (!value->IsCompatable(ie->Keys[i].second)) throw runtime_error("Type expected and given mismatch.");
                            mi.MemBook.BookVariable(loop_id, ie->Keys[i].first, move(value));
                        }
                    }
                    else if constexpr (is_same_v<T, unique_ptr<LanVariable>>) {
                        auto& value = get<unique_ptr<LanVariable>>(valueVariant);
                        //CommandLineInterface::DebugPrint("Running loop with '" + ie->Keys[0].first + "' set as: " + value->ToString());
                        if (!value->IsCompatable(ie->Keys[0].second)) throw runtime_error("Type expected and given mismatch.");
                        mi.MemBook.BookVariable(loop_id, ie->Keys[0].first, move(value));
                    };
                    
                    }, valueVariant);
                // Run the logic of the loop.
                mi.InterpretBlock(loop_id, m[2].str());
            }

            return nullopt;
        }
    },
    // While loop
    {
        regex(R"(^while\s*\((.*)\)\s*do\s*(.*))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
            while (true) {
                auto condition_var = mi.ParseParameter(scopeId, m[1].str());
                if (!condition_var.has_value()) throw runtime_error("Cannot use undefined in while loop.");
                if (condition_var.value()->Type != LanType(LanTypeEnum::TypeBool))
                    throw runtime_error("Must use boolean statement in while loop. Got " + condition_var.value()->Type.ToString());
                if (!get<bool>(condition_var.value()->Value)) break;
                mi.InterpretBlock(scopeId + ".while", m[2].str());
            }
            return nullopt;
        }
    }
};

// CODE //
MylangeInterpreter::MylangeInterpreter()
{
    this->BlockCounter = 1;
};



optional <unique_ptr< LanVariable >> MylangeInterpreter::Interpret(const string & scopeId, const string & code)
{
    smatch match;
    for (const auto& rule : rules) {
        if (regex_search(code, match, rule.pattern)) {
            return rule.action(match, *this, scopeId);
        }
    }
    return nullopt;
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
            std::string code = Utils::MakeHexCode(counter++);
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

optional<unique_ptr<LanVariable>> MylangeInterpreter::InterpretBlock(const string& scopeId, const string& blockString, const bool SkipClearing)
{
    // Cache stuff

    std::string condensed_block = CondenseBlocks(
        blockString, '{', '}', &this->BlockMap, this->BlockCounter
    );

    //std::cout << "Result:\n" << condensed_block << "\n\n";
    //std::cout << "Map contents:\n";
    for (const auto& [k, v] : this->BlockMap) {
        CommandLineInterface::DebugPrint(k + " -> [" + v + "]");
    }

    // Split into lines
    vector<string> lines = Utils::SplitString(condensed_block, ';');

    for (int i = 0; i < lines.size(); ++i) {
        string line = Utils::TrimString(lines[i]);
        if (line.empty()) continue;
		CommandLineInterface::DebugPrint("[" + scopeId + "] Interpreting line: " + line);
        auto res = this->Interpret(scopeId, line);
        if (res) {
            return res;
        }
    }
    if (!SkipClearing)
        this->MemBook.ClearScope(scopeId);
    return nullopt;
}

const regex wordCharsOnly(R"(^[a-zA-Z]\w+$)", std::regex_constants::ECMAScript);

optional<unique_ptr<LanVariable>> MylangeInterpreter::ParseParameter(const string& scopeId, const string& rawParamStr)
{
	string paramStr = Utils::TrimString(rawParamStr);
	CommandLineInterface::DebugPrint("Parsing parameter: " + paramStr);
    unique_ptr<LanVariable> result;
    smatch match;
    // Possible variable reference (soley word chars)
    if (regex_match(paramStr, match, wordCharsOnly))
    {
        CommandLineInterface::DebugPrint("Possible variable found: " + paramStr, 1);
        if (this->MemBook.GetVariable(scopeId, paramStr, result)) {
            CommandLineInterface::DebugPrint("Found variable: " + paramStr, 1);
            return move(result);
        }
        else throw runtime_error("Variable not found: " + paramStr);
	}
	// Possible function call
    else if (regex_search(paramStr, match, functionCallStack)) {
        auto res = this->RunFunctionStack(scopeId, paramStr);
        if (res) return std::move(res);
        return make_unique<LanVariable>();
		//return this->RunFunctionStack(scopeId, paramStr).value_or(make_unique<LanVariable>());
    }
    // Failsafe Random Type Conversion
    else if (this->RandomTypeConversion(scopeId, paramStr, result))
    {
        return result;
    }
    // Arithmetic Statement
    else if (LanArithmetic::IsValidLogicString(paramStr)) {
        CommandLineInterface::DebugPrint("Found Arithmetic Statement: " + paramStr);
        return move(LanArithmetic::BuildAST(paramStr)->Evaluate(*this, scopeId));
    }

    return make_unique<LanVariable>();
}

bool MylangeInterpreter::RandomTypeConversion(const string& scopeId, const string& value, unique_ptr<LanVariable>& var)
{
    optional<unique_ptr<LanVariable>> test = move(this->RandomTypeConversion(scopeId, value));
    if (test.has_value())
    {
        var = move(test.value());
        return true;
    }
    return false;
}

optional<unique_ptr<LanVariable>> MylangeInterpreter::RandomTypeConversion(const string& scopeId, const string& value)
{
    CommandLineInterface::DebugPrint("Attempting to convert value: " + value);
    string trimmedValue = Utils::TrimString(value);
    smatch matchedMatch;
    // nil
    if (trimmedValue == "nil")
    {
        CommandLineInterface::DebugPrint("Found nil");
        return make_unique<LanVariable>(
            LanType(LanTypeEnum::TypeNil),
            LanVariable::LanValue{});
    }
    // bool
    else if (trimmedValue == "true" || trimmedValue == "false")
    {
        CommandLineInterface::DebugPrint("Found bool");
        return make_unique<LanVariable>(
            LanType(LanTypeEnum::TypeBool),
            LanVariable::LanValue{ trimmedValue == "true" }
        );
    }
    // int
    else if (regex_match(trimmedValue, regex(R"(^-?\d+$)")))
    {
        CommandLineInterface::DebugPrint("Found int");
        return make_unique<LanVariable>(
            LanType(LanTypeEnum::TypeInt),
            LanVariable::LanValue{ stoi(trimmedValue) }
        );
    }
    //float
    else if (regex_match(trimmedValue, regex(R"(^-?\d+\.\d+$)")))
    {
        CommandLineInterface::DebugPrint("Found float");
        // Placeholder implementation
        return make_unique<LanVariable>(
            LanType(LanTypeEnum::TypeUnknown),
            LanVariable::LanValue{ }
        );
    }
    // char
    else if (regex_match(trimmedValue, regex(R"(^'.'$)")))
    {
        CommandLineInterface::DebugPrint("Found char");
        return make_unique<LanVariable>(
            LanType(LanTypeEnum::TypeChar),
            LanVariable::LanValue{ trimmedValue[1] }
        );
    }
    // string
    else if (regex_match(trimmedValue, regex(R"(^".*"$)")))
    {
        CommandLineInterface::DebugPrint("Found str");
        return make_unique<LanVariable>(
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
        vector<unique_ptr<LanVariable>> elements;
        for (auto& elemStr : elementStrings) {
            string trimmedElemStr = Utils::TrimString(elemStr);
            CommandLineInterface::DebugPrint("Array element string: " + trimmedElemStr, 1);
            optional <unique_ptr<LanVariable>> elemVar = this->ParseParameter(scopeId, trimmedElemStr);
            if (elemVar.has_value()) {
                //ISSUE
                elements.push_back(move(elemVar.value()));
                //elements.push_back(LanVariable(elemVar.value()->Type, move(elemVar.value()->Value)));
            }
            else {
                throw runtime_error("Failed to parse array element: " + trimmedElemStr);
            }
        }
        return make_unique<LanVariable>(
            LanType(LanTypeEnum::TypeArray),
            LanVariable::LanValue{ move(elements) }
        );
    }
    // set

    // casting

    // Iterable
    else if (regex_match(trimmedValue, matchedMatch, LanIterableEngine::RegexMatch)) {
        bool unpacking_type = false;
        vector<pair<string, LanType>> keys;
        if (Utils::IsWrappedByParens(Utils::TrimString(matchedMatch[1]), '[', ']')) {
            unpacking_type = true;
            string bare_elements = Utils::TrimString(matchedMatch[1]);
            bare_elements = bare_elements.substr(1, bare_elements.length() - 2);
            for (auto& bare_ele : Utils::TopLevelSplit(bare_elements, ',')) {
                smatch key_parts;
                regex_match(bare_ele, key_parts, paramStringPattern);
                keys.push_back({key_parts[3].str(), LanType::FromString(key_parts[2].str())});
            }
        }
        else {
            smatch key_parts;
            string s = Utils::TrimString(matchedMatch[1]);
            regex_match(s, key_parts, paramStringPattern);
            keys.push_back({ key_parts[3].str(), LanType::FromString(key_parts[2].str()) });
        }
        auto matrix_value = this->ParseParameter(scopeId, matchedMatch[2].str());
        if (matrix_value.has_value()) {
            return make_unique<LanVariable>(
                LanType(LanTypeEnum::TypeInterable),
                LanVariable::LanValue{ make_unique<LanIterableEngine>(keys, move(matrix_value.value())) }
            );
        }
        else throw runtime_error("Tryed to obtain a null value for Iterable."); 
    }
    // unknown
    else return nullopt;
};

std::vector<std::string> splitDotParenAware(const std::string& input)
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


static void MakeParameters(MylangeInterpreter& mi, const string& scopeId, string paramString,
    vector<unique_ptr<LanVariable>>& paramsOut, vector<LanType>& paramTypesOut)
{
    vector<string> param_strs = Utils::TopLevelSplit(paramString, ',');
    for (const auto& param_str : param_strs) {
        auto param = mi.ParseParameter(scopeId, Utils::TrimString(param_str));
        if (param.has_value()) {
            auto& p = param.value();
            paramsOut.push_back(move(p));
        }
        else
        {
            throw runtime_error("Failed to parse function argument: " + param_str);
        }
    }
    for (const auto& param : paramsOut) paramTypesOut.push_back(param->Type);
}


optional<unique_ptr<LanVariable>> MylangeInterpreter::RunFunctionStack(const string& scopeId, const string& functionStackStr)
{
    CommandLineInterface::DebugPrint("Executing function call(s): " + functionStackStr);
    auto function_calls = splitDotParenAware(functionStackStr);
	for (auto& fc : function_calls) {
        CommandLineInterface::DebugPrint("Function call part: " + fc, 1);
    }
    optional<unique_ptr<LanVariable>> last_result = make_unique<LanVariable>();
    string prefix = "";

    for (const auto& func_call : function_calls) {
		string function_location_scope_id =  (prefix.empty()) ? scopeId : prefix;
        //string scopeId = scopeIdRaw;
        smatch func_match;
        if (regex_search(func_call, func_match, functionPartsPattern)) {
            // find function
            string func_name = func_match[1];
            vector<unique_ptr<LanVariable>> params;
            vector<LanType> param_types;
			MakeParameters(*this, scopeId, func_match[2], params, param_types);
            // Type Literal approch first.
            if (LanFunction* func = this->MemBook.GetFunction(function_location_scope_id, func_name, param_types))
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
        else if (find(this->ImportedPackages.begin(), this->ImportedPackages.end(), func_call) != this->ImportedPackages.end())
        {
			CommandLineInterface::DebugPrint("Package reference found in function stack: " + func_call);
			prefix += func_call;
        }
		// Finally, throw error
        else throw runtime_error("Invalid function call syntax: " + func_call);
    }
    return last_result;
}

CodeBlock::CodeBlock(const string& myScopeId)
{
	this->MyScopeID = myScopeId;
}
