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
    return topLevelOps > 0;
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
            return LanVariable::String(get<string>(lhs.Value) + get<string>(rhs.Value));
        }
		// String and int: repeat string
        if (lhs.Type == LanTypeEnum::TypeString && rhs.Type == LanTypeEnum::TypeInt) {
            string result = "";
            for (int i = 0; i < std::get<int>(rhs.Value); i++)
                result += std::get<string>(lhs.Value);
            return LanVariable::String(result);
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
			return LanVariable::String(result);
        }
		// Two arrays: concatenate
		if (lhs.Type.IsArrayType() && rhs.Type.IsArrayType()) {
			LanArray result;
			const auto& lhsArr = std::get<LanArray>(lhs.Value);
			const auto& rhsArr = std::get<LanArray>(rhs.Value);
			result.reserve(lhsArr.size() + rhsArr.size());
			result.insert(result.end(), lhsArr.begin(), lhsArr.end());
			result.insert(result.end(), rhsArr.begin(), rhsArr.end());
			return LanVariable::Array(result);
		}
    }
    if (op == "<") return lhs < rhs;
	if (op == ">") return lhs > rhs;
	if (op == "<=") return lhs <= rhs;
	if (op == ">=") return lhs >= rhs;
    if (op == "&&" || op == "and")
        return LanVariable::Bool(lhs && rhs);
    if (op == "||" || op == "or")
        return LanVariable::Bool(lhs || rhs);

    throw std::runtime_error("Unknown operator: " + lhs.Type.ToString() + " " + op + " " + rhs.Type.ToString());
}