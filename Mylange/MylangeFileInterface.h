#pragma once

#include <string>
#include <optional>
#include "LanVariable.h"

class FileInterface
{
public:
	static std::string CleanFile(const std::string& filePath);
	static std::optional<std::shared_ptr<LanVariable>> InterpretFile(const std::string& filePath, bool ignoreMessage = false);
};