#include <Core/src/win/ChilWin.h>
#include <Core/src/ioc/Container.h> 
#include <Core/src/log/SeverityLevelPolicy.h> 
#include <Core/src/win/Boot.h>
#include <Core/src/log/Log.h> 
#include <Core/src/win/IWindow.h>
#include <format>
#include <ranges>
#include "CliOptions.h"

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

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PSTR pCmdLine,
	int nCmdShow)
{
	Boot();

	if (auto code = opt::Init()) {
		chilog.error(utl::ToWide(opt::GetDiagnostics())).no_trace();
		return *code;
	}
	auto& opts = opt::Get();

	if (opts.shitTheBed) {
		chilog.error(L"consider the bed thoroughly shat upon");
		return -69;
	}

	auto windowPtrs = vi::iota(0, opts.numWindows ? *opts.numWindows : 2) |
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