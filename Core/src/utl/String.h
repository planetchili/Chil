#pragma once
#include <string>

namespace chil::utl
{
	std::wstring ToWide(const std::string& narrow);
	std::string ToNarrow(const std::wstring& wide);
	std::string CamelToKebab(const std::string& camel);
}