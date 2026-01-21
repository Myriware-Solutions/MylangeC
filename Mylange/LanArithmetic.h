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
        {"==", 1}, {"!=", 1},
        {"<",  2}, {">",  2}, {"<=", 2}, {">=", 2},
        {"+",  3}, {"-",  3},
        {"*",  4}, {"/",  4}, {"%",  4},
        {"..", 5}
    };

    static const vector<string> Operators() {
        vector<string> ops;
        for (auto& pair : Precedence) {
            ops.push_back(pair.first);
        }
        return ops;
    };

    struct ExprNode {
        virtual ~ExprNode() = default;
        virtual unique_ptr<LanVariable> Evaluate(MylangeInterpreter& mi,
            const std::string& scopeId) = 0;
    };

    struct ValueNode : ExprNode {
        std::string text;

        explicit ValueNode(std::string t)
            : text(std::move(t)) {
        }

        unique_ptr<LanVariable> Evaluate(MylangeInterpreter& mi,
            const std::string& scopeId) override
        {
            // This is where YOU define meaning:
            // - literal number?
            // - variable lookup?
            // - function call?

            auto it = mi.ParseParameter(scopeId, text);
			if (it.has_value()) return move(it.value());
			else throw std::runtime_error("Error in Arithmetics, unable to evaluate value: " + text);
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

        unique_ptr<LanVariable> Evaluate(MylangeInterpreter& mi,
            const std::string& scopeId) override
        {
            // 1️⃣ Recursively evaluate children
            unique_ptr<LanVariable> lhs = left->Evaluate(mi, scopeId);
            unique_ptr<LanVariable> rhs = right->Evaluate(mi, scopeId);

            // 2️⃣ Apply operator logic
            return ApplyOperator(op, move(lhs), move(rhs));
        }
    };

    static unique_ptr<LanVariable> ApplyOperator(
        const std::string& op,
        const unique_ptr<LanVariable> lhs,
		const unique_ptr<LanVariable> rhs);

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

