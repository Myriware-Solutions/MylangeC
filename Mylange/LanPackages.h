#pragma once
#include <unordered_map>
#include <vector>
#include <string>

#include "LanFunction.h"
#include "IncludableFunctions.h"

using namespace std;

class LanPackages
{
public:
	inline static const unordered_map<string, vector<string>> Packages = {
		{"standard", {"to_string(any)", "to_int(str)"}},
		{"io", {"print(any)", "input(str)"}}
	};

	static bool HasPackage(const string& name, unique_ptr<vector<string>>& out) {
		if (Packages.contains(name)) {
			out = make_unique<vector<string>>(Packages.at(name));
			return true;
		}
		return false;
	};
};

