#include <memory>

#include "LanArithmetic.h"
#include "Utils.h"
#include "LanVariable.h"
#include "LanType.h"

bool LanArithmetic::IsValidLogicString(const std::string& input)
{
    std::string s = Utils::TrimString(input);
    if (s.empty())
        return false;

    int parenDepth = 0;
    bool expectingValue = true;

    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];

        // Track parentheses
        if (c == '(') {
            parenDepth++;
            expectingValue = true;
            continue;
        }
        if (c == ')') {
            parenDepth--;
            if (parenDepth < 0)
                return false;
            expectingValue = false;
            continue;
        }

        // Skip whitespace
        if (isspace(c))
            continue;

        // Check operators only at top level
        if (parenDepth == 0)
        {
            bool matchedOp = false;
            for (const auto& op : LanArithmetic::Operators())
            {
                if (s.compare(i, op.size(), op) == 0)
                {
                    if (expectingValue)
                        return false; // operator where value expected

                    expectingValue = true;
                    i += op.size() - 1;
                    matchedOp = true;
                    break;
                }
            }
            if (matchedOp)
                continue;
        }

        // Value characters
        if (expectingValue)
            expectingValue = false;
    }

    return parenDepth == 0 && !expectingValue;
}



LanVariable LanArithmetic::ApplyOperator(
    const std::string& op,
    const LanVariable& lhs,
    const LanVariable& rhs)
{
    if (op == "+") return lhs + rhs;
    if (op == "-") return lhs - rhs;
    if (op == "*") return lhs * rhs;
    if (op == "/") return lhs / rhs;
    if (op == "==") return lhs == rhs;
    if (op == "..") {
        if (lhs.Type == LanType::BaseTypes::TypeString && rhs.Type == LanType::BaseTypes::TypeString) {
            return LanVariable(LanType(LanType::BaseTypes::TypeString), get<string>(lhs.Value) + get<string>(rhs.Value));
        }
        if (lhs.Type == LanType::BaseTypes::TypeString && rhs.Type == LanType::BaseTypes::TypeInt) {
            string result = "";
            for (int i = 0; i < get<int>(rhs.Value); i++) result += get<string>(lhs.Value);
            return LanVariable(LanType(LanType::BaseTypes::TypeString), result);
        }
    }
    //if (op == "<") return lhs < rhs;

    throw std::runtime_error("Unknown operator: " + op);
}