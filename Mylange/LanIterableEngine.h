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
	vector<vector<unique_ptr<LanVariable>>> Values;
	vector<string> Keys;

	LanIterableEngine(std::vector<std::string> keys, std::unique_ptr < LanVariable > matrixVar);

	vector<unordered_map<string, LanVariable>> GetIterable();
};