#include <Core/src/win/ChilWin.h>
#include <objbase.h>
#include <Core/src/ioc/Container.h> 
#include <Core/src/log/SeverityLevelPolicy.h> 
#include <Core/src/log/Log.h> 
#include <Core/src/win/Boot.h>
#include <Core/src/gfx/IResourceLoader.h>
#include <Core/src/gfx/ISpriteBatcher.h>
#include <Core/src/gfx/d12/Boot.h>
#include "Global.h"
#include "ActiveWindow.h"
#include <ranges>
#include <chrono>

using namespace chil;
using namespace std::string_literals;
using namespace std::chrono_literals;
using hrclock = std::chrono::high_resolution_clock;
namespace rn = std::ranges;
namespace vi = rn::views;


void Boot()
{
	log::Boot();
	ioc::Get().Register<log::ISeverityLevelPolicy>([] {
		return std::make_shared<log::SeverityLevelPolicy>(log::Level::Info);
	});

	win::Boot();

	gfx::d12::Boot();
}

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PWSTR pCmdLine,
	int nCmdShow)
{
	try {
		// init COM
		if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
			throw std::runtime_error{ "COM farked" };
		}

		// initialize services in ioc containers
		Boot();

		// shortcut for ioc container
		auto& C = ioc::Get();
		// create sprite codex
		auto pSpriteCodex = C.Resolve<gfx::ISpriteCodex>({ Global::nSheets });
		// create resource loader
		auto pLoader = C.Resolve<gfx::IResourceLoader>();
		// load sprite atlases (textures) into sprite codex
		{
			std::vector<gfx::IResourceLoader::FutureTexture> futures;
			for (int i = 0; i < Global::nSheets; i++) {
				futures.push_back(pLoader->LoadTexture(std::format(L"sprote-shiet-{}.png", i)));
			}
			for (auto& f : futures) {
				pSpriteCodex->AddSpriteAtlas(f.get());
			}
		}

		auto windows = vi::iota(0, Global::nWindows) |
			vi::transform([&](int i) {return std::make_unique<ActiveWindow>(i, pSpriteCodex); }) |
			rn::to<std::vector>();

		while (!windows.empty()) {
			std::erase_if(windows, [](auto& p) {return !p->IsLive(); });
			std::this_thread::sleep_for(50ms);
		}
	}
	catch (const std::exception& e) {
		chilog.error(L"Error caught at top level: " + utl::ToWide(e.what())).no_trace();
		return -1;
	}

	return 0;
}