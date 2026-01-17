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


using namespace std;

const regex functionPartsPattern(R"(^([\w.]+)\s*\((.*)\))", std::regex_constants::ECMAScript);
const regex functionCallStack(R"(^(?:\w+\.)+(?:\w+\(.*\))+)", std::regex_constants::ECMAScript);
const regex functionDotExtention(R"((\)\.))", std::regex_constants::ECMAScript);
const regex ifElseThenPattern(R"(^if\s*\((.*?)\)\s*then\s*(.*))", std::regex_constants::ECMAScript);

struct Rule {
    regex pattern;
    function <optional<LanVariable>(const smatch&, MylangeInterpreter&, const string&) > action;
};

vector<Rule> rules = {
    // Return
    {
        regex(R"(return\s+(.*))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
            auto it = mi.ParseParameter(scopeId, m[1].str());
            if (it.has_value()) return it.value();
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
                for (auto& func_name : *package_functions) {
                    unique_ptr<LanFunction> func;
                    if (IncludableFunctions::GetIncludableFunction(func_name, func))
                    {
                        CommandLineInterface::DebugPrint("Included package function: " + func_name + " : " + (func)->GetId());
						string func_scope_id = scopeId + ".pkg." + package_name;
                        mi.MemBook.BookFunction(func_scope_id, std::move(func));
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
			LanType expectedType = LanType::FromString(m[1]);
            //auto lv = LanVariable::RandomTypeConversion(m[3]);
			auto lv = mi.ParseParameter(scopeId, m[3]);
            if (!lv.has_value()) 
                throw runtime_error("Failed to parse variable value.");
			CommandLineInterface::DebugPrint("Setting variable " + m[2].str() + " of type " + expectedType.ToString() + "/" + lv.value().Type.ToString() + " to value " + lv.value().ToString());
            if (lv.value().Type == expectedType)
			    mi.MemBook.BookVariable(scopeId, m[2], lv.value());
			else throw runtime_error("Type mismatch in variable assignment. Expected " 
                + expectedType.ToString() + ", got " + lv.value().Type.ToString() 
                + " (with " + lv.value().ToString() + ")");
            return nullopt;
        }
    },
    // Do Block
    {
        regex(R"(^do\s+(.*))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
			CommandLineInterface::DebugPrint("[" + scopeId + "] Interpreting block: " + m[1].str());
			mi.InterpretBlock(scopeId, m[1]);
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
                    if (conditionEval.has_value() && conditionEval->Type.BaseType == LanType::BaseTypes::TypeBool
                        && get<bool>(conditionEval->Value))
                    {
						return mi.InterpretBlock(scopeId, match[2].str());
                    }
                }
                else {
					return mi.InterpretBlock(scopeId, part);
                }
            }
        }
    }
};

// CODE //
MylangeInterpreter::MylangeInterpreter()
{
    this->BlockCounter = 1;
};



optional<LanVariable> MylangeInterpreter::Interpret(const string& scopeId, const string& code)
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

optional<LanVariable> MylangeInterpreter::InterpretBlock(const string& scopeId, const string& blockString)
{
    // Cache stuff

    std::string condensed_block = CondenseBlocks(
        blockString, '{', '}', &this->BlockMap, this->BlockCounter
    );

    //std::cout << "Result:\n" << condensed_block << "\n\n";
    //std::cout << "Map contents:\n";
    for (const auto& [k, v] : this->BlockMap) {
        CommandLineInterface::DebugPrint(k + " -> [" + v + "]\n");
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
    return nullopt;
}

optional<LanVariable> MylangeInterpreter::ParseParameter(const string& scopeId, const string& rawParamStr)
{
	string paramStr = Utils::TrimString(rawParamStr);
	CommandLineInterface::DebugPrint("Parsing parameter: " + paramStr);
	LanVariable result;
    smatch match;
    // Possible variable reference
    if (this->MemBook.GetVariable(scopeId, paramStr, result))
    {
        return result;
	}
	// Possible function call
    else if (regex_search(paramStr, match, functionCallStack)) {
		return this->RunFunctionStack(scopeId, paramStr).value_or(LanVariable());
    }
    // Failsafe Random Type Conversion
	else if (LanVariable::RandomTypeConversion(paramStr, &result))
    {
        return result;
    }
    // Arithmetic Statement
    else if (LanArithmetic::IsValidLogicString(paramStr)) {
        CommandLineInterface::DebugPrint("Found Arithmetic Statement: " + paramStr);
        return LanArithmetic::BuildAST(paramStr)->Evaluate(*this, scopeId);
    }

    return LanVariable();
}

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


optional<LanVariable> MylangeInterpreter::RunFunctionStack(const string& scopeId, const string& functionStackStr)
{
    CommandLineInterface::DebugPrint("Executing function call(s): " + functionStackStr);
	auto function_calls = Utils::resplit(functionStackStr, functionDotExtention);
    optional<LanVariable> last_result = nullopt;



    for (const auto& func_call : function_calls) {
        smatch func_match;
        if (regex_search(func_call, func_match, functionPartsPattern)) {
            // find function
            string func_name = func_match[1];
            vector<string> arg_strs = splitDotParenAware(func_match[2]);
            vector<LanVariable> args;
            for (const auto& arg_str : arg_strs) {
                auto arg = this->ParseParameter(scopeId, Utils::TrimString(arg_str));
                if (arg.has_value()) {
                    args.push_back(arg.value());
                }
                else
                {
                    throw runtime_error("Failed to parse function argument: " + arg_str);
				}
            }
            vector<LanType> arg_types;
            for (const auto& arg : args) arg_types.push_back(arg.Type);
            // Type Literal approch first.
            if (LanFunction* func = this->MemBook.GetFunction(scopeId, func_name, arg_types))
            {
                CommandLineInterface::DebugPrint("Function " + func_name + " exists and is running...");
                last_result = (func)->Execute(scopeId, *this, args);
            }
            // Then, try to see if there is an overload with the "any" type in unmatched positions.
            else {
                vector<LanFunction*> overloads = this->MemBook.GetFunctionOverloads(scopeId, func_name);
				CommandLineInterface::DebugPrint("Function " + func_name + " has " + to_string(overloads.size()) + " overloads.");
                bool found = false;
                for (auto& overload : overloads) {
                    const auto& param_map = overload->Parameters;
                    if (param_map.size() != args.size()) continue;
                    bool match = true;
                    auto param_it = param_map.begin();
                    for (size_t i = 0; i < args.size(); ++i, ++param_it) {
                        if (!(args[i].Type == param_it->second ||
                            param_it->second.BaseType == LanType::BaseTypes::TypeAny)) {
                            match = false;
                            break;
                        }
                    }
                    if (match) {
                        CommandLineInterface::DebugPrint("Function " + func_name + " overload exists and is running...");
                        last_result = overload->Execute(scopeId, *this, args);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    string err = "Function not found: " + func_name + " with parameter types: ";
				}
			}
        }
        else throw runtime_error("Invalid function call syntax: " + func_call);
    }
    return last_result;
}

CodeBlock::CodeBlock(const string& myScopeId)
{
	this->MyScopeID = myScopeId;
}
