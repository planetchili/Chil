#include <Core/src/win/ChilWin.h>
#include <Core/src/ioc/Container.h> 
#include <Core/src/ioc/Singletons.h>
#include <Core/src/log/SeverityLevelPolicy.h> 
#include <Core/src/win/Boot.h>
#include <Core/src/log/Log.h> 
#include <Core/src/win/Window.h>
#include <format>

using namespace chil;
using namespace std::chrono_literals;

void Boot()
{
	log::Boot();
	ioc::Get().Register<log::ISeverityLevelPolicy>([] {
		return std::make_shared<log::SeverityLevelPolicy>(log::Level::Info);
	});

	win::Boot();
}

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PWSTR pCmdLine,
	int nCmdShow)
{
	Boot();

	win::Window window{ ioc::Sing().Resolve<win::IWindowClass>(), L"And he morbed all over them", { 800, 600 } };

	int x = 0;
	while (!window.IsClosing()) {
		std::this_thread::sleep_for(100ms);
		window.SetTitle(std::format(L"Happy Window [{:*<{}}]", L'*', x + 1));
		x = (x + 1) % 20;
	}

	return 0;
}