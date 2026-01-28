#include <memory>

#include "LanArithmetic.h"
#include "Utils.h"
#include "LanVariable.h"
#include "LanType.h"
#include "LanIterableEngine.h"

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



unique_ptr<LanVariable> LanArithmetic::ApplyOperator(
    const std::string& op,
    const unique_ptr<LanVariable> lhs,
    const unique_ptr<LanVariable> rhs)
{
    if (op == "+") return make_unique<LanVariable>(*lhs + *rhs);
    if (op == "-") return make_unique<LanVariable>(*lhs - *rhs);
    if (op == "*") return make_unique<LanVariable>(*lhs * *rhs);
    if (op == "/") return make_unique<LanVariable>(*lhs / *rhs);
    if (op == "==") return make_unique<LanVariable>(*lhs == *rhs);
    if (op == "..") {
        if (lhs->Type == LanTypeEnum::TypeString && rhs->Type == LanTypeEnum::TypeString) {
            return make_unique<LanVariable>(LanType(LanTypeEnum::TypeString), get<string>(lhs->Value) + get<string>(rhs->Value));
        }
        if (lhs->Type == LanTypeEnum::TypeString && rhs->Type == LanTypeEnum::TypeInt) {
            string result = "";
            for (int i = 0; i < get<int>(rhs->Value); i++) result += get<string>(lhs->Value);
            return make_unique<LanVariable>(LanType(LanTypeEnum::TypeString), result);
        }
    }
    if (op == "<") return make_unique<LanVariable>(*lhs < *rhs);
	if (op == ">") return make_unique<LanVariable>(*lhs > *rhs);
	if (op == "<=") return make_unique<LanVariable>(*lhs <= *rhs);
	if (op == ">=") return make_unique<LanVariable>(*lhs >= *rhs);
    if (op == "&&" || op == "and")
        return make_unique<LanVariable>(
            LanType(LanTypeEnum::TypeBool),
            LanVariable::LanValue{ *lhs && *rhs }
        );
    if (op == "||" || op == "or")
        return make_unique<LanVariable>(
            LanType(LanTypeEnum::TypeBool),
            LanVariable::LanValue{ *lhs || *rhs }
        );

    throw std::runtime_error("Unknown operator: " + op);
}