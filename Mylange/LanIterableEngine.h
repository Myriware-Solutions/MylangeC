// LanIterableEngine.h
#pragma once
#include "LanType.h"
#include "LanVariable.h"
#include <bit>
#include <bitset>
#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>


class LanIterableEngine {
public:

	inline static const regex RegexMatch = regex(R"((.*?)\s*(?:\bin\b|::)\s*(.*))");

	std::vector<std::variant<LanArray, LanVariable> > Values;
	std::vector<std::pair<std::string, LanType>> Keys;
	bool IsUnpackingIter;

	LanIterableEngine(std::vector<std::pair<std::string,LanType>> keys, LanVariable matrixVar);
};