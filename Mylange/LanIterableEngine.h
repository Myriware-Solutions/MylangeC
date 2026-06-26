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

	inline static const regex RegexMatch = regex(R"((.*?)\s*(?:\bin\b|::)\s*(.*))");

	std::vector<std::variant<LanArray, LanVariable> > Values;
	std::vector<std::pair<std::string, LanType>> Keys;
	bool IsUnpackingIter;

	LanIterableEngine(std::vector<std::pair<std::string,LanType>> keys, LanVariable matrixVar);
};