#include <iostream>
#include <Core/src/log/Log.h>
#include <Core/src/ioc/Container.h>
#include <Core/src/log/SeverityLevelPolicy.h>
#include <Core/src/utl/Assert.h>
#include <Core/src/win/WindowClass.h>
#include <Core/src/ccr/GenericTaskQueue.h>


using namespace chil;
using namespace std::string_literals;

void Boot()
{
	log::Boot();

	ioc::Get().Register<log::ISeverityLevelPolicy>([] {
		return std::make_shared<log::SeverityLevelPolicy>(log::Level::Info);
	});
}


void f()
{
	chilog.error(L"oops!");
}

int main(int argc, const char** argv)
{
	Boot();

	auto pWinClass = std::make_shared<win::WindowClass>();

	chilog.error(L"look ma, no trace").no_trace().no_line();
	chilog.info().trace();

	ccr::GenericTaskQueue q;
	q.Push([] {});

	//chilog.fatal(L"Oh noes!");
	//chilog.warn(L"huh");
	//f();

	int x = 0, y = 1;
	//chilass(x > y).msg(L"butts").ass_watch(x, y, rand());

	//chilchk(x > y);

	try {
		chilchk(x > y).ass_watch(x).ex();
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	return 0;
}