#pragma once
#include <string>

using namespace std;

class Utils
{
public:
	static string TrimString(const string& str)
	{
		size_t first = str.find_first_not_of(' ');
		if (first == string::npos)
			return "";
		size_t last = str.find_last_not_of(' ');
		return str.substr(first, (last - first + 1));
	}
};

