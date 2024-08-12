#include "String.h"
#include <ranges>
#include <algorithm>
#include <cctype>

namespace rn = std::ranges;
namespace vi = std::views;

namespace chil::utl
{
	std::wstring ToWide(const std::string& narrow)
	{
		std::wstring wide;
		wide.resize(narrow.size() + 1);
		size_t actual;
		mbstowcs_s(&actual, wide.data(), wide.size(), narrow.c_str(), _TRUNCATE);
		if (actual > 0)
		{
			wide.resize(actual - 1);
			return wide;
		}
		return {};
	}

	std::string ToNarrow(const std::wstring& wide)
	{
		std::string narrow;
		narrow.resize(wide.size() * 2);
		size_t actual;
		wcstombs_s(&actual, narrow.data(), narrow.size(), wide.c_str(), _TRUNCATE);
		narrow.resize(actual - 1);
		return narrow;
	}

	std::string CamelToKebab(const std::string& camel)
	{
		const auto tf = [](char c) { return std::isupper(c) ?
			std::string{ '-', (char)std::tolower(c) } : std::string{ c }; };
		return camel | vi::transform(tf) | vi::join | rn::to<std::basic_string>();
	}
}