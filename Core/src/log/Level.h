#pragma once
#include <string>

namespace chil::log
{
	enum class Level
	{
		Trace,
		Debug,
		Info,
		Warn,
		Error,
		Fatal,
	};

	std::wstring GetLevelName(Level);
}