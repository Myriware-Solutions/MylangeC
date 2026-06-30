#pragma once

#include <string>

class FileInterface
{
public:
	static std::string CleanFile(const std::string& filePath);
	static int InterpretFile(const std::string& filePath);
};