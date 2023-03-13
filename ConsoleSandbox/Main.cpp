#include <iostream>
#include <Core/src/log/Log.h>
#include <Core/src/ioc/Container.h>
#include <Core/src/log/SeverityLevelPolicy.h>
#include <Core/src/utl/Assert.h>


using namespace chil;
using namespace std::string_literals;

void Boot()
{
	log::Boot();

	ioc::Get().Register<log::ISeverityLevelPolicy>([] {
		return std::make_shared<log::SeverityLevelPolicy>(log::Level::Warn);
	});
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

	int x = 0, y = 1;
	chilass(x > y).msg(L"butts").ass_watch(x).ass_watch(y);

	return 0;
}