#include "ChilCppUnitTest.h"
#include <Core/src/log/EntryBuilder.h>
#include <Core/src/log/Channel.h>
#include <Core/src/log/Driver.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace chil;
using namespace std::string_literals;

#define chilog log::EntryBuilder{ __FILEW__, __FUNCTIONW__, __LINE__ }

class MockDriver : public log::IDriver
{
public:
	void Submit(const log::Entry& e) override
	{
		entry_ = e;
	}
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
	TEST_CLASS(LogChannelTests)
	{
	public:
		// test channel forwarding entries to driver
		TEST_METHOD(TestForwarding)
		{
			log::Channel chan;
			auto pDriver1 = std::make_shared<MockDriver>();
			auto pDriver2 = std::make_shared<MockDriver>();
			chan.AttachDriver(pDriver1);
			chan.AttachDriver(pDriver2);
			chilog.info(L"HI").chan(&chan);
			Assert::AreEqual(L"HI"s, pDriver1->entry_.note_);
			Assert::AreEqual(log::Level::Info, pDriver1->entry_.level_);
			Assert::AreEqual(L"HI"s, pDriver2->entry_.note_);
			Assert::AreEqual(log::Level::Info, pDriver2->entry_.level_);
		}
	};
}
