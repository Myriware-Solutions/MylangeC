#pragma once
#include <unordered_map>
#include<string>
#include<functional>

class LanVariable;

#include "LanVariable.h"

class TypeFunctions
{
public:
	inline static std::unordered_map<std::string, std::function<LanVariable>> String = {};
	static void Register(const std::string& name, std::function<LanVariable> func) {
		String[name] = move(func);
	}

};