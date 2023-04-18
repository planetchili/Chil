#pragma once
#include "TextFormatter.h"
#include "Entry.h"
#include <format>
#include <sstream>
#include <Core/src/win/Utilities.h>

namespace chil::log
{
	std::wstring TextFormatter::Format(const Entry& e) const
	{
		std::wostringstream oss;
		oss << std::format(L"@{} {{{}}} {}\n  >> at {}\n     {}({})\n",
			GetLevelName(e.level_),
			std::chrono::zoned_time{ std::chrono::current_zone(), e.timestamp_ },
			e.note_,
			e.sourceFunctionName_,
			e.sourceFile_,
			e.sourceLine_
		);
		if (e.hResult_) {
			oss << std::format(L"  !HRESULT [{:#010x}]: {}\n", *e.hResult_,
				win::GetErrorDescription(*e.hResult_));
		}
		if (e.trace_) {
			oss << e.trace_->Print() << std::endl;
		}
		return oss.str();
	}
}