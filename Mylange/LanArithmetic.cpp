#include <memory>

#include "LanArithmetic.h"
#include "Utils.h"
#include "LanVariable.h"
#include "LanType.h"
#include "LanIterableEngine.h"
#include <cctype>
#include <stdexcept>
#include <string>
#include <variant>
#include "CommandLineInterface.h"

bool LanArithmetic::IsValidLogicString(const std::string& input) {
    std::string s = Utils::TrimString(input);
    CommandLineInterface::DebugPrint("Checking for arithmetic: " + s);
    
    if (s.empty()) return false;

    int depth = 0;
    int topLevelOps = 0;

    const auto& ops = LanArithmetic::Operators();
    const auto& ops_chars = LanArithmetic::OperatorCharacters();
    /*for (const auto& op : ops) {
        CommandLineInterface::DebugPrint("Op registered: " + op, 1);
    }
    for (const auto& op_c : ops_chars) {
        string sc{ op_c };
        CommandLineInterface::DebugPrint("Op char registered: " + sc, 1);
    }*/
    
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        string ss{ c };
        //CommandLineInterface::DebugPrint("Char: " + ss, 1);
        if (isspace(c)) continue;
        
        bool found = false;
        if (Utils::Find(ops_chars, c)) {
            //CommandLineInterface::DebugPrint("Found op char");
            // Possible operator sequence
            for (const auto& op : ops) {
                //CommandLineInterface::DebugPrint("Checking op compat: " + op);
                bool found_match = true;
                for (size_t j = 0; j < op.length(); j++) {
                    if (i + op.length() > s.length()) {
                        //CommandLineInterface::DebugPrint("Overflow detected");
                        found_match = false;
                        break;
                    }
                    if (s[i + j] != op[j]) {
                        //CommandLineInterface::DebugPrint("These do not match:");
                        found_match = false;
                        break;
                    }
                }
                if (!found_match) {
                    continue;
                }
                else {
                    topLevelOps++;
                    found = true;
                    break;
                }
            }
        }

        if (!found && Utils::IsOpeningBracket(c)) {
            depth++;
            continue;
        }
        else if (!found && Utils::IsClosingBracket(c)) {
            depth--;
            continue;
        }

        // Else, its just a charactere
    }

    //CommandLineInterface::DebugPrint("Found many top-level operators: " + to_string(topLevelOps));
    return topLevelOps > 0;
}



bool dummy(const std::string& input)
{
    std::string s = Utils::TrimString(input);
    
    if (s.empty())
        return false;

    int parenDepth = 0;
    bool expectingValue = true;
    bool foundTopLevelOperator = false;
    bool inValueToken = false;

    const auto& ops = LanArithmetic::Operators();

    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];

        if (isspace(static_cast<unsigned char>(c))) {
            inValueToken = false;
            continue;
        }

        if (c == '(') {
            if (!expectingValue) return false;
            parenDepth++;
            inValueToken = false;
            continue;
        }

        if (c == ')') {
            if (expectingValue) return false;
            parenDepth--;
            if (parenDepth < 0) return false;
            expectingValue = false;
            inValueToken = false;
            continue;
        }

        //  operator matching ONLY at token boundary
        if (parenDepth == 0 && !expectingValue && !inValueToken)
        {
            for (const auto& op : ops)
            {
                if (s.compare(i, op.size(), op) == 0)
                {
                    foundTopLevelOperator = true;
                    expectingValue = true;
                    i += op.size() - 1;
                    goto next_char;
                }
            }
        }

        // value token
        if (expectingValue) {
            expectingValue = false;
            inValueToken = true;
            continue;
        }

        // continuation of value token
        inValueToken = true;

    next_char:
        continue;
    }

    return parenDepth == 0
        && !expectingValue
        && foundTopLevelOperator;
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
		// Two strings: concatenate
        if (lhs.Type == LanTypeEnum::TypeString && rhs.Type == LanTypeEnum::TypeString) {
            return LanVariable(LanType(LanTypeEnum::TypeString), get<string>(lhs.Value) + get<string>(rhs.Value));
        }
		// String and int: repeat string
        if (lhs.Type == LanTypeEnum::TypeString && rhs.Type == LanTypeEnum::TypeInt) {
            string result = "";
            for (int i = 0; i < std::get<int>(rhs.Value); i++)
                result += std::get<string>(lhs.Value);
            return LanVariable(LanType(LanTypeEnum::TypeString), result);
        }
        // Array and string: concat with string/char delim
        if (lhs.Type.IsArrayType() && (rhs.Type == LanTypeEnum::TypeString || rhs.Type == LanTypeEnum::TypeChar)) {
			string result = "";
			for (auto& item : std::get<LanArray>(lhs.Value)) {
                result += item->ToString();
                if (item != std::get<LanArray>(lhs.Value).back()) {
                    if (rhs.Type == LanTypeEnum::TypeString) result += std::get<string>(rhs.Value);
					else if (rhs.Type == LanTypeEnum::TypeChar) result += std::get<char>(rhs.Value);
                }
			}
			return LanVariable(LanType(LanTypeEnum::TypeString), result);
        }
		// Two arrays: concatenate
		if (lhs.Type.IsArrayType() && rhs.Type.IsArrayType()) {
			LanArray result;
			const auto& lhsArr = std::get<LanArray>(lhs.Value);
			const auto& rhsArr = std::get<LanArray>(rhs.Value);
			result.reserve(lhsArr.size() + rhsArr.size());
			result.insert(result.end(), lhsArr.begin(), lhsArr.end());
			result.insert(result.end(), rhsArr.begin(), rhsArr.end());
			return LanVariable(LanType(LanTypeEnum::TypeArray), result);
		}
    }
    if (op == "<") return lhs < rhs;
	if (op == ">") return lhs > rhs;
	if (op == "<=") return lhs <= rhs;
	if (op == ">=") return lhs >= rhs;
    if (op == "&&" || op == "and")
        return LanVariable(
            LanType(LanTypeEnum::TypeBool),
            LanVariable::LanValue{ lhs && rhs }
        );
    if (op == "||" || op == "or")
        return LanVariable(
            LanType(LanTypeEnum::TypeBool),
            LanVariable::LanValue{ lhs || rhs }
        );

    throw std::runtime_error("Unknown operator: " + op);
}