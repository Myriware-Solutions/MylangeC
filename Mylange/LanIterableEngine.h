// LanIterableEngine.h
#pragma once
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <optional>
#include <variant>
#include <functional>
#include <type_traits>
#include <bitset>
#include <bit>

#include "LanVariable.h"
#include "LanType.h"


class LanIterableEngine {
public:

	inline static const regex RegexMatch = regex(R"((.*?)\s*\bin\b\s*(.*))");

	vector< variant<vector<unique_ptr<LanVariable>>, unique_ptr<LanVariable>> > Values;
	std::vector<std::pair<std::string, LanType>> Keys;
	bool IsUnpackingIter;

	LanIterableEngine(std::vector<std::pair<std::string,LanType>> keys, std::unique_ptr < LanVariable > matrixVar);

	bool GetIterable(vector<unordered_map<string, unique_ptr<LanVariable>>>& vectorOut);
};