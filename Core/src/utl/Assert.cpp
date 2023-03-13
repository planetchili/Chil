#include "Assert.h" 
#include <Core/src/log/Log.h>


namespace chil::utl
{
	namespace {
#ifdef NDEBUG
		constexpr int skip_depth = 3;
		constexpr int skip_depth_ex = 23;
#else
		constexpr int skip_depth = 7;
		constexpr int skip_depth_ex = 29;
#endif
	}

	Assertion::Assertion(std::wstring expression, const wchar_t* file, const wchar_t* function, int line, Consequence consequence)
		:
		file_{ file },
		function_{ function },
		line_{ line },
		consequence_{ consequence },
		skip_depth_{ skip_depth }
	{
		stream_ << L"Assertion Failed! " << expression << "\n";
	}
	Assertion::~Assertion()
	{
		log::EntryBuilder{ file_, function_, line_ }
			.trace_skip(skip_depth_)
			.chan(log::GetDefaultChannel())
			.level(consequence_ == Consequence::Terminate ? log::Level::Fatal : log::Level::Error)
			.note(stream_.str());
		if (consequence_ == Consequence::Terminate) {
			log::GetDefaultChannel()->Flush();
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
		skip_depth_ = skip_depth_ex;
		throw FailedAssertion{};
	}
}