// IMPORTS //
#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <functional>
#include "MylangeInterpreter.h"
#include "CommandLineInterface.h"
#include "MemoryBooker.h"
#include "LanType.h"

using namespace std;

struct Rule {
    regex pattern;
    function<void(const smatch&, MylangeInterpreter&, const string&)> action;
};

vector<Rule> rules = {
    {
        std::regex(R"(foo)"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
            std::cout << "Matched foo\n";
        }
    },
    {
        std::regex(R"(bar(\d+))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
            std::cout << "Bar value: " << m[1] << "\n";
        }
    },
    {
        regex(R"(^\s*([a-zA-Z<>,|\s]+) +(\w+) *=> *(.*))"),
        [](auto const& m, MylangeInterpreter& mi, const string& scopeId) {
			LanType expectedType = LanType::FromString(m[1]);
            LanVariable lv = LanVariable::RandomTypeConversion(m[3]);
            if (lv.Type == expectedType)
			    mi.MemBook.BookVariable(scopeId, m[2], lv);
			else throw runtime_error("Type mismatch in variable assignment.");
        }
    }
};

// CODE //
MylangeInterpreter::MylangeInterpreter()
{
    this->Counter = 0;
};

int MylangeInterpreter::Interpret(const string& scopeId, const string& code)
{
    smatch match;
    for (const auto& rule : rules) {
        if (regex_search(code, match, rule.pattern)) {
            rule.action(match, *this, scopeId);
        }
    }
    return 0;
};





CodeBlock::CodeBlock(const string& myScopeId)
{
	this->MyScopeID = myScopeId;
}
