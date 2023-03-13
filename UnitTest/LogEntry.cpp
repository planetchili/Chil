#include "ChilCppUnitTest.h"
#include <Core/src/log/EntryBuilder.h>
#include <Core/src/log/Channel.h>
#include <Core/src/log/Policy.h>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace chil;
using namespace std::string_literals;

#define chilog log::EntryBuilder{ __FILEW__, __FUNCTIONW__, __LINE__ }

class MockChannel : public log::IChannel
{
public:
	void Submit(log::Entry& e) override
	{
		entry_ = e;
	}
	void AttachDriver(std::shared_ptr<log::IDriver>) override {}
	void Flush() override {}
	void AttachPolicy(std::shared_ptr<log::IPolicy>) override {}
	log::Entry entry_;
};

template<> inline std::wstring __cdecl
Microsoft::VisualStudio::CppUnitTestFramework::
	ToString<log::Level>(const log::Level& level)
{
	return log::GetLevelName(level);
}

namespace Log
{
	TEST_CLASS(LogEntryTests)
	{
	public:
		// temporary test to show off fluid interface
		TEST_METHOD(ShowOffFluent)
		{
			MockChannel chan;
			chilog.level(log::Level::Info).note(L"HI").chan(&chan);
			Assert::AreEqual(L"HI"s, chan.entry_.note_);
			Assert::AreEqual(log::Level::Info, chan.entry_.level_);
		}
		// testing simplified level/note
		TEST_METHOD(SimplifiedLevelNote)
		{
			MockChannel chan;
			chilog.info(L"HI").chan(&chan);
			Assert::AreEqual(L"HI"s, chan.entry_.note_);
			Assert::AreEqual(log::Level::Info, chan.entry_.level_);
		}
	};
}
