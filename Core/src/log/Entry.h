#pragma once
#include "Level.h"
#include <chrono>
#include <optional>
#include <Core/src/utl/StackTrace.h>

namespace chil::log
{
	struct Entry
	{
		// data fields 
		Level level_ = Level::Error;
		std::wstring note_;
		const wchar_t* sourceFile_ = nullptr;
		const wchar_t* sourceFunctionName_ = nullptr;
		int sourceLine_ = -1;
		std::chrono::system_clock::time_point timestamp_;
		std::optional<utl::StackTrace> trace_;
		std::optional<unsigned int> hResult_;
		// behavior override flags 
		std::optional<bool> captureTrace_;
		std::optional<bool> showSourceLine_;
	};
}