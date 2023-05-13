#include "ChilCppUnitTest.h"
#include <Core/src/log/Entry.h>
#include <Core/src/log/TextFormatter.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace chil;
using namespace std::string_literals;

namespace Log
{
	TEST_CLASS(LogTextFormatterTests)
	{
	public:
		// testing text formatting
		TEST_METHOD(TestFormat)
		{
			const log::Entry e{
				.level_ = log::Level::Info,
				.note_ = L"Heya",
				.sourceFile_ = L"C:\\Users\\Chili\\Desktop\\cpp\\Chil\\UnitTest\\LogTextFormatter.cpp",
				.sourceFunctionName_ = __FUNCTIONW__,
				.sourceLine_ = __LINE__,
				.timestamp_ = std::chrono::system_clock::time_point{
					std::chrono::days{ 10'000 }
				}
			};

			std::wstring expectedText = std::format(L"@Info {{{0}}} Heya\n  >> at Log::LogTextFormatterTests::TestFormat\n     {1}({2})\n",
				std::chrono::zoned_time{ std::chrono::current_zone(), e.timestamp_ }, e.sourceFile_, e.sourceLine_);

			Assert::AreEqual(expectedText, log::TextFormatter{}.Format(e));
		}
	};
}
