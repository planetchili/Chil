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
				.sourceFile_ = __FILEW__,
				.sourceFunctionName_ = __FUNCTIONW__,
				.sourceLine_ = __LINE__,
				.timestamp_ = std::chrono::system_clock::time_point{
					std::chrono::days{ 10'000 }
				}
			};
			Assert::AreEqual(
				L"@Info {1997-05-19 09:00:00.0000000 GMT+9} Heya\n  >> at Log::LogTextFormatterTests::TestFormat\n     C:\\Users\\Chili\\Desktop\\cpp\\Chil\\UnitTest\\LogTextFormatter.cpp(23)\n"s,
				log::TextFormatter{}.Format(e)
			);
		}
	};
}
