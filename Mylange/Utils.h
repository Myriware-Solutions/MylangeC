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

    template <typename T>
    static bool Find(const vector<T>& list, const T& item) {
        return (std::find(list.begin(), list.end(), item) != list.end());
    }

    static std::vector<std::string> GetKeysSortedByLengthDesc(const std::unordered_map<std::string, int>& map)
    {
        std::vector<std::string> keys;
        keys.reserve(map.size());

        for (const auto& [key, _] : map)
            keys.push_back(key);

        std::sort(keys.begin(), keys.end(),
            [](const std::string& a, const std::string& b) {
                return a.size() > b.size();
            });

        return keys;
    }

	static string JoinStrings(const vector<string>& strings, const string& delimiter) {
		if (strings.empty()) return "";
		ostringstream oss;
		for (size_t i = 0; i < strings.size() - 1; ++i) {
			oss << strings[i] << delimiter;
		}
		oss << strings.back();
		return oss.str();
	}

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

    static string MakeHexCode(string prefix, size_t id) {
        ostringstream oss;
        oss << prefix << uppercase << hex
            << setw(8) << setfill('0') << id;
        return oss.str();
    }

    static std::vector<int> SplitFlags(int flags)
    {
        std::vector<int> result;

        for (int bit = 1; bit != 0; bit <<= 1)
        {
            if ((flags & bit) == bit)
                result.push_back(bit);
        }

        return result;
    }

    inline static const std::vector<std::pair<char, char>>& BracketPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'}, {'<', '>'}
    };

    static const bool IsAlphanumeric(const std::string& str) {
        return !str.empty() && std::all_of(str.begin(), str.end(), [](unsigned char c) {
            return std::isalnum(c);
            });
    }

    static const bool IsOpeningBracket(char& c, vector<char> except) {
        for (auto& pair : BracketPairs) {
            if (c == pair.first && !Find(except, c)) return true;
        }
        return false;
    }

    static const bool IsOpeningBracket(char& c) {
        for (auto& pair : BracketPairs) {
            if (c == pair.first) return true;
        }
        return false;
    }

    static const bool IsClosingBracket(char& c, vector<char> except) {
        for (auto& pair : BracketPairs) {
            if (c == pair.second && !Find(except, c)) return true;
        }
        return false;
    }

    static const bool IsClosingBracket(char& c) {
        for (auto& pair : BracketPairs) {
            if (c == pair.second) return true;
        }
        return false;
    }

    static std::vector<std::string> TopLevelSplit(
        const std::string& input,
        char delimiter
    ) {
        std::vector<std::string> result;
        std::unordered_map<char, int> depth;

        for (auto& b : BracketPairs) {
            depth[b.first] = 0;  // track open brackets
        }

        std::string current;

        for (char c : input) {
            // check opens
            for (auto& b : BracketPairs) {
                if (c == b.first) {
                    depth[b.first]++;
                    current.push_back(c);
                    goto continue_loop;
                }
            }

            // check closes
            for (auto& b : BracketPairs) {
                if (c == b.second) {
                    depth[b.first]--;
                    current.push_back(c);
                    goto continue_loop;
                }
            }

            // check delimiter at top-level
            {
                bool topLevel = true;
                for (auto& b : BracketPairs) {
                    if (depth[b.first] > 0) {
                        topLevel = false;
                        break;
                    }
                }

                if (topLevel && (c == delimiter)) {
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

    static std::vector<std::string> TopLevelSplitStringDep(
        const std::string& input,
        const std::string& delimiter
    ) {
        std::vector<std::string> result;
        std::unordered_map<char, int> depth;

        for (auto& b : BracketPairs) {
            depth[b.first] = 0;
        }

        std::string current;
        size_t i = 0;

        while (i < input.size()) {
            char c = input[i];

            // check opens
            for (auto& b : BracketPairs) {
                if (c == b.first) {
                    depth[b.first]++;
                    current.push_back(c);
                    ++i;
                    goto continue_loop;
                }
            }

            // check closes
            for (auto& b : BracketPairs) {
                if (c == b.second) {
                    depth[b.first]--;
                    current.push_back(c);
                    ++i;
                    goto continue_loop;
                }
            }

            // check delimiter at top level
            {
                bool topLevel = true;
                for (auto& b : BracketPairs) {
                    if (depth[b.first] > 0) {
                        topLevel = false;
                        break;
                    }
                }

                if (topLevel &&
                    !delimiter.empty() &&
                    input.compare(i, delimiter.size(), delimiter) == 0)
                {
                    result.push_back(current);
                    current.clear();
                    i += delimiter.size();
                    goto continue_loop;
                }
            }

            // normal character
            current.push_back(c);
            ++i;

        continue_loop:;
        }

        if (!current.empty())
            result.push_back(current);

        return result;
    }


    static std::vector<std::string> TopLevelSplit(
        const std::string& input,
        vector<char> delimiter
    ) {
        std::vector<std::string> result;
        std::unordered_map<char, int> depth;

        for (auto& b : BracketPairs) {
            depth[b.first] = 0;  // track open brackets
        }

        std::string current;

        for (char c : input) {
            // check opens
            for (auto& b : BracketPairs) {
                if (c == b.first) {
                    depth[b.first]++;
                    current.push_back(c);
                    goto continue_loop;
                }
            }

            // check closes
            for (auto& b : BracketPairs) {
                if (c == b.second) {
                    depth[b.first]--;
                    current.push_back(c);
                    goto continue_loop;
                }
            }

            // check delimiter at top-level
            {
                bool topLevel = true;
                for (auto& b : BracketPairs) {
                    if (depth[b.first] > 0) {
                        topLevel = false;
                        break;
                    }
                }

                if (topLevel && (find(delimiter.begin(), delimiter.end(), c) != delimiter.end())) {
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

    static std::vector<std::string> TopLevelSplit(
        const std::string& input,
        const std::string& delimiter,
        bool requireWordBoundary = false
    )
    {
        std::vector<std::string> parts;
        std::string current;

        // Track nesting depth per bracket type
        std::unordered_map<char, char> openToClose;
        std::unordered_map<char, char> closeToOpen;
        std::unordered_map<char, int> depth;

        for (auto& p : BracketPairs)
        {
            openToClose[p.first] = p.second;
            closeToOpen[p.second] = p.first;
            depth[p.first] = 0;
        }

        auto isWordChar = [](char c)
            {
                return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
            };

        auto atWordBoundary = [&](size_t pos)
            {
                bool left = (pos == 0) ||
                    !isWordChar(input[pos - 1]);
                bool right = (pos + delimiter.size() >= input.size()) ||
                    !isWordChar(input[pos + delimiter.size()]);
                return left && right;
            };

        for (size_t i = 0; i < input.size();)
        {
            char c = input[i];

            // Opening bracket
            if (openToClose.count(c))
            {
                ++depth[c];
                current += c;
                ++i;
                continue;
            }

            // Closing bracket
            if (closeToOpen.count(c))
            {
                char open = closeToOpen[c];
                --depth[open];
                current += c;
                ++i;
                continue;
            }

            // Check if we are at top level for ALL brackets
            bool atTopLevel = true;
            for (auto& d : depth)
            {
                if (d.second > 0)
                {
                    atTopLevel = false;
                    break;
                }
            }

            // Attempt delimiter match
            if (atTopLevel &&
                !delimiter.empty() &&
                input.compare(i, delimiter.size(), delimiter) == 0 &&
                (!requireWordBoundary || atWordBoundary(i)))
            {
                parts.push_back(current);
                current.clear();
                i += delimiter.size();
                continue;
            }

            // Normal character
            current += c;
            ++i;
        }

        if (!current.empty())
            parts.push_back(current);

        return parts;
    }

    static bool IsWrappedByParens(const std::string& s, const char& begin, const char& end) {
        if (s.size() < 2 || s.front() != begin || s.back() != end)
            return false;

        int depth = 0;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == begin) depth++;
            else if (s[i] == end) {
                depth--;
                if (depth == 0 && i != s.size() - 1)
                    return false;
            }
        }
        return depth == 0;
    };
};