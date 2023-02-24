#include <iostream>
#include <Core/src/log/EntryBuilder.h>
#include <Core/src/log/Channel.h>
#include <Core/src/log/MsvcDebugDriver.h>
#include <Core/src/log/TextFormatter.h>
#include <Core/src/log/SeverityLevelPolicy.h>
#include <Core/src/log/Log.h>


using namespace chil;
using namespace std::string_literals;

void Boot()
{
	log::Boot();
}


void f()
{
	chilog.error(L"oops!");
}

int main()
{
	Boot();

	chilog.fatal(L"Oh noes!");
	chilog.warn(L"huh");
	f();

	utl::StackTrace st;
	auto st2 = std::move(st);

	std::wcout << st2.Print() << std::endl;
	std::wcout << st.Print() << std::endl;

	return 0;
}