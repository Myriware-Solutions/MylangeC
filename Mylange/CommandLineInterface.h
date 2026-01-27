#pragma once
class CommandLineInterface
{
public:
	static inline bool DebugEnabled = false;

	static void RunCLI();
	static void DebugPrint(const std::string& message, const int& indent = 0);
};

