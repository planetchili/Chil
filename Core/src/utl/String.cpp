#include "String.h"

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
}