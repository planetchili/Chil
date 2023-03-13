#include "Assert.h" 
#include <Core/src/log/Log.h>

namespace chil::utl
{
	Assertion::Assertion(std::wstring expression, const wchar_t* file, const wchar_t* function, int line, Consequence consequence)
		:
		file_{ file },
		function_{ function },
		line_{ line },
		consequence_{ consequence }
	{
		stream_ << L"Assertion Failed! " << expression << "\n";
	}
	Assertion::~Assertion()
	{
		log::EntryBuilder{ file_, function_, line_ }
			.trace_skip(7)
			.chan(log::GetDefaultChannel())
			.level(consequence_ == Consequence::Terminate ? log::Level::Fatal : log::Level::Error)
			.note(stream_.str());
		if (consequence_ == Consequence::Terminate) {
			std::terminate();
		}
	}
	Assertion& Assertion::msg(const std::wstring& message)
	{
		stream_ << L"  Msg: " << message << L"\n";
		return *this;
	}
	void Assertion::ex()
	{
		consequence_ = Consequence::Exception;
		throw FailedAssertion{};
	}
}