#pragma once
#include <string>

namespace chil::log
{
	enum class Level
	{
		None,
		Fatal,
		Error,
		Warn,
		Info,
		Debug,
		Verbose,
	};

	std::wstring GetLevelName(Level);
}