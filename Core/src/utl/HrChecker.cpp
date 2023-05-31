#include "HrChecker.h"
#include <Core/src/win/Utilities.h>
#include <Core/src/utl/String.h>
#include <ranges>
#include <format>

namespace rn = std::ranges;
namespace vi = rn::views;


namespace chil::utl
{
	CheckerToken chk;

	HrGrabber::HrGrabber(unsigned int hr, std::source_location loc) noexcept
		:
		hr(hr),
		loc(loc)
	{}
	void operator>>(HrGrabber g, CheckerToken)
	{
		if (FAILED(g.hr)) {
			// get error description as narrow string with crlf removed
			auto errorString = utl::ToNarrow(win::GetErrorDescription(g.hr)) |
				vi::transform([](char c) {return c == '\n' ? ' ' : c; }) |
				vi::filter([](char c) {return c != '\r'; }) |
				rn::to<std::basic_string>();
			throw std::runtime_error{
				std::format("Graphics Error: {}\n   {}({})",
				errorString, g.loc.file_name(), g.loc.line())
			};
		}
	}
}