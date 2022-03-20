#include <cctype>
#include <algorithm>

#include <Utility/String.hpp>

namespace String
{
	bool startsWith(const std::string& str, const std::string& prefix)
	{
		return str.find(prefix) == 0;
	}

	std::string reverse(const std::string& str)
	{
		auto result = str;
		std::reverse(result.begin(), result.end());
		return result;
	}
}