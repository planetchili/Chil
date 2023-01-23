#pragma once
#include "Level.h"
#include <chrono>

namespace chil::log
{
	struct Entry
	{
		Level level_;
		std::wstring note_;
		const wchar_t* sourceFile_;
		const wchar_t* sourceFunctionName_;
		int sourceLine_;
		std::chrono::system_clock::time_point timestamp_;
	};
}