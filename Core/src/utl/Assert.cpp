#include "Assert.h" 
#include <Core/src/log/Log.h>

namespace chil::utl
{
	Assertion::Assertion(std::wstring expression, const wchar_t* file, const wchar_t* function, int line)
		:
		file_{ file },
		function_{ function },
		line_{ line }
	{
		stream_ << L"Assertion Failed! " << expression << "\n";
	}
	Assertion::~Assertion()
	{
		log::EntryBuilder{ file_, function_, line_ }
			.trace_skip(7)
			.chan(log::GetDefaultChannel())
			.fatal(stream_.str());
		std::terminate();
	}
	Assertion& Assertion::msg(const std::wstring& message)
	{
		stream_ << L"  Msg: " << message << L"\n";
		return *this;
	}
}