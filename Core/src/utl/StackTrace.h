#pragma once
#include <memory>
#include <string>

namespace backward
{
	class StackTrace;
}

namespace chil::utl
{
	class StackTrace
	{
	public:
		StackTrace();
		StackTrace(const StackTrace& src);
		StackTrace& operator=(const StackTrace& src);
		~StackTrace();
		std::wstring Print() const;

	private:
		std::unique_ptr<backward::StackTrace> pTrace;
	};
}