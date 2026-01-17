#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iterator>
#include <stdexcept>
#include <iomanip>
#include <unordered_map>

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

    static vector<string> SplitString(const string& s, char delimiter) {
        vector<string> result;
        string current;

        for (char c : s) {
            if (c == delimiter) {
                result.push_back(current);
                current.clear();
            }
            else {
                current += c;
            }
        }

        result.push_back(current); // last token
        return result;
    }

    static vector<string> SplitString(const string& s, const string& delimiter) {
        vector<string> result;
        size_t pos = 0;
        size_t delim_len = delimiter.length();
        size_t start = 0;
        while ((pos = s.find(delimiter, start)) != string::npos) {
            result.push_back(s.substr(start, pos - start));
            start = pos + delim_len;
        }
        result.push_back(s.substr(start)); // last token
        return result;
	}

    /**
     * Splits a string into a vector of strings using a regular expression as a delimiter.
     *
     * @param s The input string.
     * @param sep_regex The regular expression for the delimiter.
     * @return A vector containing the split substrings.
     */
    static std::vector<std::string> resplit(const std::string& s, const std::regex& sep_regex) {
        // The -1 parameter tells the iterator to return the unmatched parts (the tokens)
        std::sregex_token_iterator iter(s.begin(), s.end(), sep_regex, -1);
        std::sregex_token_iterator end; // Default constructed iterator signifies the end of the sequence

        // Construct a vector from the iterator range
        return { iter, end };
    }

    static string ReadFileContents(const string& path) {
        ifstream file(path, ios::binary);
        if (!file)
            throw runtime_error("Failed to open file");

        return string(
            (istreambuf_iterator<char>(file)),
            istreambuf_iterator<char>()
        );
    }

    static string MakeHexCode(size_t id) {
        ostringstream oss;
        oss << "0x" << uppercase << hex
            << setw(3) << setfill('0') << id;
        return oss.str();
    }

    static std::vector<std::string> TopLevelSplit(
        const std::string& input,
        char delimiter,
        const std::vector<std::pair<char, char>>& brackets = {
            {'(', ')'}, {'[', ']'}, {'{', '}'}, {'<', '>'}
        }
    ) {
        std::vector<std::string> result;
        std::unordered_map<char, int> depth;

        for (auto& b : brackets) {
            depth[b.first] = 0;  // track open brackets
        }

        std::string current;

        for (char c : input) {
            // check opens
            for (auto& b : brackets) {
                if (c == b.first) {
                    depth[b.first]++;
                    current.push_back(c);
                    goto continue_loop;
                }
            }

            // check closes
            for (auto& b : brackets) {
                if (c == b.second) {
                    depth[b.first]--;
                    current.push_back(c);
                    goto continue_loop;
                }
            }

            // check delimiter at top-level
            {
                bool topLevel = true;
                for (auto& b : brackets) {
                    if (depth[b.first] > 0) {
                        topLevel = false;
                        break;
                    }
                }

                if (topLevel && c == delimiter) {
                    result.push_back(current);
                    current.clear();
                    goto continue_loop;
                }
            }

            // normal character
            current.push_back(c);

        continue_loop:; // label target
        }

        // push last part
        if (!current.empty())
            result.push_back(current);

        return result;
    }
};

