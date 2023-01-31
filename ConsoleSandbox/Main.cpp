#include <iostream>
#include <Core/src/log/EntryBuilder.h>
#include <Core/src/log/Channel.h>
#include <Core/src/log/MsvcDebugDriver.h>
#include <Core/src/log/TextFormatter.h>


using namespace chil;
using namespace std::string_literals;

#define chilog log::EntryBuilder{ __FILEW__, __FUNCTIONW__, __LINE__ }.chan(pChan.get())

int main()
{
	std::unique_ptr<log::IChannel> pChan = std::make_unique<log::Channel>(std::vector<std::shared_ptr<log::IDriver>>{
		std::make_shared<log::MsvcDebugDriver>(std::make_unique<log::TextFormatter>())
	});
	chilog.fatal(L"Oh noes!");

	return 0;
}