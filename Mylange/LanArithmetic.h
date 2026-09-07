#pragma once
#include "LanVariable.h"
#include "MylangeInterpreter.h"
#include "Utils.h"
#include <climits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

class LanArithmetic
{
public:
    inline static const unordered_map<std::string, int> Precedence = {
        {"and", 1}, {"&&", 1}, {"or", 1}, {"||", 1},
        {"==", 2}, {"!=", 2},
        {"<",  3}, {">",  3}, {"<=", 3}, {">=", 3},
        {"+",  4}, {"-",  4},
        {"*",  5}, {"/",  5}, {"%",  5},
        {"^", 6}, {"..", 6}
    };

    static const vector<string> Operators() {
        return Utils::GetKeysSortedByLengthDesc(Precedence);
    };

    static const vector<char> OperatorCharacters() {
        vector<char> res;
        for (const auto& op : Operators()) {
            for (const char c : op) {
                if (!Utils::Find(res, c))
                    res.push_back(c);
            }
        }
        return res;
    }

    static const bool IsOperator(string op) {
        return Utils::Find(LanArithmetic::Operators(), op);
    }

    struct ExprNode {
        virtual ~ExprNode() = default;
        virtual std::shared_ptr<LanVariable> Evaluate(MylangeInterpreter& mi) = 0;
    };

    struct ValueNode : ExprNode {
        std::string text;

        explicit ValueNode(std::string t)
            : text(std::move(t)) {
        }

        std::shared_ptr<LanVariable> Evaluate(MylangeInterpreter& mi) override
        {
            auto it = mi.ParseParameter(text);
			if (it.has_value()) return it.value();
			else throw std::runtime_error("Error in Arithmetics, unable to evaluate value: '" + text + "'");
        }
    };

    struct BinaryNode : ExprNode {
        std::string op;
        std::unique_ptr<ExprNode> left;
        std::unique_ptr<ExprNode> right;

        BinaryNode(std::string o,
            std::unique_ptr<ExprNode> l,
            std::unique_ptr<ExprNode> r)
            : op(std::move(o)), left(std::move(l)), right(std::move(r)) {
        }

        std::shared_ptr<LanVariable> Evaluate(MylangeInterpreter& mi) override
        {
            // Recursively evaluate children
            auto lhs = left->Evaluate(mi);
            auto rhs = right->Evaluate(mi);
            // Apply operator logic
            return ApplyOperator(op, lhs, rhs);
        }
    };

    static std::shared_ptr<LanVariable> ApplyOperator(
        const std::string& op,
        const std::shared_ptr<LanVariable> lhs,
		const std::shared_ptr<LanVariable> rhs);

    static unique_ptr<LanArithmetic::ExprNode> BuildAST(const string& expr)
    {
        std::string s = Utils::TrimString(expr);
        if (s.empty())
            return nullptr;

        // Strip outer parentheses
        while (Utils::IsWrappedByParens(s, '(', ')')) {
            s = Utils::TrimString(s.substr(1, s.size() - 2));
        }

        // Returns true if c is a character that can be part of an identifier/word
        auto IsWordChar = [](char c) {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
            };

        // Checks that matching `op` at s[pos..pos+op.size()) doesn't land
        // inside a larger identifier/word (e.g. "or" inside "for",
        // "and" inside "operand", etc.)
        auto HasWordBoundary = [&](const std::string& str, size_t pos, const std::string& op) {
            // Only word-like operators need boundary checks ("and", "or", "not", "mod", ...).
            // Symbolic operators ("+", "-", "==", ...) don't suffer from this ambiguity.
            bool isWordOp = !op.empty() && (std::isalpha(static_cast<unsigned char>(op.front())) || op.front() == '_');
            if (!isWordOp)
                return true;

            // Character immediately before the match must not be a word char
            if (pos > 0 && IsWordChar(str[pos - 1]))
                return false;

            // Character immediately after the match must not be a word char
            size_t endPos = pos + op.size();
            if (endPos < str.size() && IsWordChar(str[endPos]))
                return false;

            return true;
            };

        int depth = 0;
        int bestPos = -1;
        int bestPrec = INT_MAX;
        std::string bestOp;

        for (size_t i = 0; i < s.size(); ++i)
        {
            char c = s[i];

            if (c == '(') depth++;
            else if (c == ')') depth--;

            if (depth != 0)
                continue;

            for (const auto& [op, prec] : Precedence)
            {
                if (s.compare(i, op.size(), op) == 0)
                {
                    if (!HasWordBoundary(s, i, op))
                        continue; // matched inside a larger word — skip it

                    if (prec <= bestPrec) {
                        bestPrec = prec;
                        bestPos = (int)i;
                        bestOp = op;
                    }
                }
            }
        }

        // No operator found -> value node
        if (bestPos == -1) {
            return std::make_unique<ValueNode>(s);
        }

        // Split at operator
        std::string leftStr = s.substr(0, bestPos);
        std::string rightStr = s.substr(bestPos + bestOp.size());

        return std::make_unique<BinaryNode>(
            bestOp,
            BuildAST(leftStr),
            BuildAST(rightStr)
        );
    }

    static bool IsValidLogicString(const std::string& input);

};