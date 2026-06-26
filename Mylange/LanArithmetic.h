#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "MylangeInterpreter.h"
#include <climits>
#include <stdexcept>
#include <utility>
#include "LanVariable.h"
#include "Utils.h"

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
        {"..", 6}
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
        virtual LanVariable Evaluate(MylangeInterpreter& mi) = 0;
    };

    struct ValueNode : ExprNode {
        std::string text;

        explicit ValueNode(std::string t)
            : text(std::move(t)) {
        }

        LanVariable Evaluate(MylangeInterpreter& mi) override
        {
            // This is where YOU define meaning:
            // - literal number?
            // - variable lookup?
            // - function call?

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

        LanVariable Evaluate(MylangeInterpreter& mi) override
        {
            // 1️⃣ Recursively evaluate children
            LanVariable lhs = left->Evaluate(mi);
            LanVariable rhs = right->Evaluate(mi);

            // 2️⃣ Apply operator logic
            return ApplyOperator(op, lhs, rhs);
        }
    };

    static LanVariable ApplyOperator(
        const std::string& op,
        const LanVariable& lhs,
		const LanVariable& rhs);

    static unique_ptr<LanArithmetic::ExprNode> BuildAST(const string& expr)
    {
        std::string s = Utils::TrimString(expr);
        if (s.empty())
            //throw runtime_error("Empty parse.");
            return nullptr;

        // Strip outer parentheses
        while (Utils::IsWrappedByParens(s, '(', ')')) {
            s = Utils::TrimString(s.substr(1, s.size() - 2));
        }

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

