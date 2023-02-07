#define _CRT_SECURE_NO_WARNINGS
#include "StackTrace.h"
#include <sstream>
#include "String.h"

#pragma warning(push)
#pragma warning(disable : 26495 26439 26451)
#include <Core/third/backward.hpp>
#pragma warning(pop)

namespace chil::utl
{
	StackTrace::StackTrace(size_t skip)
	{
		backward::TraceResolver thisIsAWorkaround; // https://github.com/bombela/backward-cpp/issues/206
		pTrace = std::make_unique<backward::StackTrace>();
		pTrace->load_here(64);
		if (skip != 0) {
			pTrace->skip_n_firsts(skip);
		}
	}
	StackTrace::StackTrace(const StackTrace& src)
		:
		pTrace{ std::make_unique<backward::StackTrace>(*pTrace) }
	{}
	StackTrace& StackTrace::operator=(const StackTrace& src)
	{
		pTrace = std::make_unique<backward::StackTrace>(*pTrace);
		return *this;
	}
	StackTrace::~StackTrace() {}
	std::wstring StackTrace::Print() const
	{
		std::ostringstream oss;
		backward::Printer printer;
		printer.print(*pTrace, oss);
		return utl::ToWide(oss.str());
	}
}