// IMPORTS //
#include <iostream>
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
#include "MemoryBooker.h"
#include "IncludableFunctions.h"
#include "LanType.h"
#include "LanFunction.h"
#include "Utils.h"


using namespace std;

regex functionPartsPattern(R"(^(\w+)\s*\((.*)\))", std::regex_constants::ECMAScript);

struct Rule {
    regex pattern;
    function<optional<LanVariable>(const smatch&, MylangeInterpreter&, const string&)> action;
};

vector<Rule> rules = {
    //
    {
        regex(R"(^#include\s*<(\w+)>)"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
			CommandLineInterface::DebugPrint("Including module: " + m[1].str());
			LanFunction func;
			if (IncludableFunctions::GetIncludableFunction(m[1], func))
            {
                CommandLineInterface::DebugPrint("Included module: " + m[1].str());
				mi.MemBook.BookFunction(scopeId, func);
            }
            else throw runtime_error("Module not found: " + m[1].str());
            return nullopt;
        }
    },
    // Set Variable
    {
        regex(R"(^\s*([a-zA-Z<>,|\s]+) +(\w+) *=> *(.*))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
			LanType expectedType = LanType::FromString(m[1]);
            auto lv = LanVariable::RandomTypeConversion(m[3]);
            if (!lv.has_value())
				throw runtime_error("Failed to parse variable value.");
            if (lv.value().Type == expectedType)
			    mi.MemBook.BookVariable(scopeId, m[2], lv.value());
			else throw runtime_error("Type mismatch in variable assignment.");
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
            LanFunction function = LanFunction(return_type, m[2].str(), parameter_map, m[4].str());
            mi.MemBook.BookFunction(scopeId, function);
            return nullopt;
        }
    },
    // Functional Execution
    {
        regex(R"(^(?:\w+\(.*\))+)"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
			mi.RunFunctionStack(scopeId, m[0].str());
            return nullopt;
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
        std::cout << k << " -> [" << v << "]\n";
    }

    // Split into lines
    vector<string> lines = Utils::SplitString(condensed_block, ';');

    for (int i = 0; i < lines.size(); ++i) {
        string line = Utils::TrimString(lines[i]);
        if (line.empty()) continue;
		cout << "Running line " << (i + 1) << ": " << line << endl;
        auto res = this->Interpret(scopeId, line);
        if (res) {
            return res;
        }
    }
    return nullopt;
}

LanVariable MylangeInterpreter::ParseParameter(const string& scopeId, const string& paramStr)
{
	CommandLineInterface::DebugPrint("Parsing parameter: " + paramStr);
	LanVariable result;
    // Possible variable reference
    if (this->MemBook.GetVariable(scopeId, paramStr, result))
    {
        return result;
	}
    // Failsafe Random Type Conversion
	else if (LanVariable::RandomTypeConversion(paramStr, &result))
    {
        return result;
    }

    return LanVariable();
}

optional<LanVariable> MylangeInterpreter::RunFunctionStack(const string& scopeId, const string& functionStackStr)
{
    CommandLineInterface::DebugPrint("Executing function call(s): " + functionStackStr);
    auto function_calls = Utils::TopLevelSplit(functionStackStr, '.');
    optional<LanVariable> last_result = nullopt;
    for (const auto& func_call : function_calls) {
        smatch func_match;
        if (regex_search(func_call, func_match, functionPartsPattern)) {
            // find function
            string func_name = func_match[1];
            vector<string> arg_strs = Utils::TopLevelSplit(func_match[2], ',');
            vector<LanVariable> args;
            for (const auto& arg_str : arg_strs)
                args.push_back(this->ParseParameter(scopeId, Utils::TrimString(arg_str)));
            vector<LanType> arg_types;
            for (const auto& arg : args) arg_types.push_back(arg.Type);
            LanFunction func;
            if (
                this->MemBook.GetFunction(scopeId, func_name, arg_types, func) ||
				IncludableFunctions::GetIncludableFunction(func_name, func)
            )
            {
                func.Execute(scopeId, *this, args);
            }
            else throw runtime_error("Function not found: " + func_name);
        }
        else throw runtime_error("Invalid function call syntax: " + func_call);
    }
    return last_result;
}

optional<LanVariable> MylangeInterpreter::RunFunctionStack(const string& scopeId, const string& name, const vector<LanVariable> parameters)
{
	throw runtime_error("Not implemented");
}
;

CodeBlock::CodeBlock(const string& myScopeId)
{
	this->MyScopeID = myScopeId;
}
