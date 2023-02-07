#pragma once
#include "Level.h"
#include <chrono>
#include <optional>
#include <Core/src/utl/StackTrace.h>

namespace chil::log
{
	struct Entry
	{
		Level level_ = Level::Error;
		std::wstring note_;
		const wchar_t* sourceFile_ = nullptr;
		const wchar_t* sourceFunctionName_ = nullptr;
		int sourceLine_ = -1;
		std::chrono::system_clock::time_point timestamp_;
		std::optional<utl::StackTrace> trace_;
	};
}