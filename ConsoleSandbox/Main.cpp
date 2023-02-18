#include <iostream>
#include <Core/src/log/EntryBuilder.h>
#include <Core/src/log/Channel.h>
#include <Core/src/log/MsvcDebugDriver.h>
#include <Core/src/log/TextFormatter.h>
#include <Core/src/log/SeverityLevelPolicy.h>


using namespace chil;
using namespace std::string_literals;

#define chilog log::EntryBuilder{ __FILEW__, __FUNCTIONW__, __LINE__ }.chan(pChan.get())

std::unique_ptr<log::IChannel> pChan;

void f()
{
	chilog.error(L"oops!");
}

int main()
{
	pChan = std::make_unique<log::Channel>(
		std::vector<std::shared_ptr<log::IDriver>>{
			std::make_shared<log::MsvcDebugDriver>(std::make_unique<log::TextFormatter>())
		}
	);
	pChan->AttachPolicy(std::make_unique<log::SeverityLevelPolicy>(log::Level::Error));
	chilog.fatal(L"Oh noes!");
	chilog.warn(L"huh");
	f();

	utl::StackTrace st;
	auto st2 = std::move(st);

	std::wcout << st2.Print() << std::endl;
	std::wcout << st.Print() << std::endl;

	return 0;
}