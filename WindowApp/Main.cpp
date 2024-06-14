#include <Core/src/win/ChilWin.h>
#include <Core/src/ioc/Container.h> 
#include <Core/src/log/SeverityLevelPolicy.h> 
#include <Core/src/win/Boot.h>
#include <Core/src/log/Log.h> 
#include <Core/src/win/IWindow.h>
#include <format>
#include <ranges> 
#include <CLI/CLI.hpp>

using namespace chil;
using namespace std::string_literals;
using namespace std::chrono_literals;
namespace rn = std::ranges;
namespace vi = rn::views;

void Boot()
{
	log::Boot();
	ioc::Get().Register<log::ISeverityLevelPolicy>([] {
		return std::make_shared<log::SeverityLevelPolicy>(log::Level::Info);
	});

	win::Boot();
}

struct CliArgs
{
	int optionOne = -1;
	bool flagA = false;
	bool flagB = false;

	CliArgs()
	{
		app.add_option("--option-one", optionOne, "It's an option");
		app.add_flag("-a,--flag-a", flagA);
		app.add_flag("-b,--flag-b", flagB);
		try {
			(app).parse(((*__p___argc())), ((*__p___argv())));
		}
		catch (const CLI::ParseError& e) {
			chilog.error(utl::ToWide(e.what()));
			std::terminate();
		};
	}
private:
	CLI::App app{ "Chil Framework Test Window Application" };
} g_cli;

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PSTR pCmdLine,
	int nCmdShow)
{
	Boot();

	chilog.info(std::format(L"{} {} {}", g_cli.optionOne, g_cli.flagA, g_cli.flagB));

	auto windowPtrs = vi::iota(0, 10) |
		vi::transform([](auto i) {return ioc::Get().Resolve<win::IWindow>(); }) |
		rn::to<std::vector>();

	int x = 0;
	while (!windowPtrs.empty()) {
		std::erase_if(windowPtrs, [](auto& p) {return p->IsClosing(); });
		for (auto& p : windowPtrs) {
			p->SetTitle(std::format(L"Happy Window [{:*<{}}]", L'*', x + 1));
		}
		x = (x + 1) % 20;
		std::this_thread::sleep_for(50ms);
	}

	return 0;
}