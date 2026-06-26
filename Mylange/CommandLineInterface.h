#pragma once
#include <exception>
#include <iostream>
#include <regex>
#include <string>

class CommandLineInterface
{
public:
	static inline bool DebugEnabled = false;

	static void RunCLI();
	static void DebugPrint(const std::string& message, const int& indent = 0)
	{
		if (!CommandLineInterface::DebugEnabled) return;
		std::string indent_str = std::string(indent * 4, ' ');
		try {
			std::cout << "[DEBUG] " << indent_str << message << std::endl;
		}
		catch (const std::exception& e) {
			std::cout << "[OUTERR] error outputting: " << e.what() << std::endl;
		};
	};
	enum class DebugColor {
		White,
		Red,
		Green,
		Yellow,
		Blue,
		Magenta,
		Cyan
	};
	static void DebugPrint(const std::string& message, const DebugColor color, const int indent = 0)
	{
		if (!CommandLineInterface::DebugEnabled) return;
		std::string indent_str = std::string(indent * 4, ' ');

		// ANSI color codes
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
			std::cout << "[DEBUG] " << indent_str << colorCode << message << reset << std::endl;
		}
		catch (const std::exception& e) {
			std::cout << "[OUTERR] error outputting: " << e.what() << std::endl;
		}
	};
};