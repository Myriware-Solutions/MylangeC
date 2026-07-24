#pragma once
#include <exception>
#include <iostream>
#include <regex>
#include <string>

class CommandLineInterface
{
protected:
	static inline const std::string Version = "Pre - Orange 0.9.3";
public:
	static inline bool DebugEnabled = false;
	static inline std::vector<std::string> Args = {};

	static void RunCLI();

	enum class DebugColor {
		White,
		Red,
		Green,
		Yellow,
		Blue,
		Magenta,
		Cyan
	};

	static void Print(const std::string& msg, const DebugColor color = DebugColor::White, const int& indent = 0)
	{
		std::string indent_str = std::string(indent * 4, ' ');
		const char* colorCode;
		switch (color) {
		case DebugColor::Red:     colorCode = "\033[31m"; break;
		case DebugColor::Green:   colorCode = "\033[32m"; break;
		case DebugColor::Yellow:  colorCode = "\033[33m"; break;
		case DebugColor::Blue:    colorCode = "\033[34m"; break;
		case DebugColor::Magenta: colorCode = "\033[35m"; break;
		case DebugColor::Cyan:    colorCode = "\033[36m"; break;
		case DebugColor::White:
		default:                  colorCode = "\033[37m"; break;
		}
		const char* reset = "\033[0m";
		try {
			std::cout << indent_str << colorCode << msg << reset << std::endl;
		}
		catch (const std::exception& e) {
			std::cout << "[OUTERR] error outputting: " << e.what() << std::endl;
		};
	};

	static void DebugPrint(const std::string& message, const int& indent = 0)
	{
		if (!CommandLineInterface::DebugEnabled) return;
		CommandLineInterface::Print(message, DebugColor::White, indent);
	};
	
	static void DebugPrint(const std::string& message, const DebugColor color, const int indent = 0)
	{
		if (!CommandLineInterface::DebugEnabled) return;
		CommandLineInterface::Print(message, color, indent);
	};
};