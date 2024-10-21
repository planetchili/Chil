#include <Core/src/win/ChilWin.h>
#include <objbase.h>
#include <Core/src/ioc/Container.h> 
#include <Core/src/log/SeverityLevelPolicy.h> 
#include <Core/src/log/Log.h> 
#include <Core/src/win/BootWin.h>
#include <Core/src/gfx/IResourceLoader.h>
#include <Core/src/gfx/ISpriteBatcher.h>
#include <Core/src/gfx/d12/BootD12.h>
#include "ActiveWindow.h"
#include <ranges>
#include <chrono>
#include <Core/src/log/Log.h> 
#include <Core/src/win/IWindow.h>
#include <format>
#include "CliOptions.h"
#include <Core/src/simp/SimpleContext.h>
#include <Core/src/gfx/SpriteFrame.h>

using namespace chil;
using namespace std::string_literals;
using namespace std::chrono_literals;
using hrclock = std::chrono::high_resolution_clock;
namespace rn = std::ranges;
namespace vi = rn::views;



void RunNormal()
{
	auto& opts = opt::Get();
	// init COM
	if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
		throw std::runtime_error{ "COM farked" };
	}
	// initialize services in ioc containers
	log::Boot();
	win::Boot();
	gfx::d12::Boot();
	// shortcut for ioc container
	auto& C = ioc::Get();
	// create sprite codex
	auto pSpriteCodex = C.Resolve<gfx::ISpriteCodex>({ 0 });
	// create resource loader
	auto pLoader = C.Resolve<gfx::IResourceLoader>();
	// load sprite atlases (textures) into sprite codex
	{
		std::vector<gfx::IResourceLoader::FutureTexture> futures;
		for (uint32_t i = 0; i < *opts.numSheets; i++) {
			futures.push_back(pLoader->LoadTexture(std::format(L"sprote-shiet-{}.png", i)));
		}
		for (auto& f : futures) {
			pSpriteCodex->AddAtlas(f.get());
		}
	}

	auto windows = vi::iota(0u, *opts.numWindows) |
		vi::transform([&](int i) {return std::make_unique<ActiveWindow>(i, pSpriteCodex); }) |
		rn::to<std::vector>();

	while (!windows.empty()) {
		std::erase_if(windows, [](auto& p) {return !p->IsLive(); });
		std::this_thread::sleep_for(50ms);
	}
}

void RunSimp()
{
	auto& opts = opt::Get();
	simp::Init({ 1280, 720 }, L"Weeeeeee Heeeeee", *opts.logLevel);
	gfx::SpriteFrame frame{ {8, 4}, {0, 0}, simp::LoadAtlas(L"sprote-shiet-0.png") };
	while (!simp::Win().IsClosing()) {
		simp::Begin();
		for (float x = -200.f; x <= 200.f; x += 10.f) {
			for (float y = -200.f; y <= 200.f; y += 20.f) {
				frame.DrawToBatch(simp::Batch(), {x, y});
			}
		}
		simp::End();
	}
}

void RunBlown()
{
	auto& opts = opt::Get();
	simp::Init({ 1280, 720 }, L"Chunbus", *opts.logLevel);
	gfx::SpriteFrame frame{ {8, 4}, {0, 0}, simp::LoadAtlas(L"sprote-shiet-0.png") };
	while (!simp::Win().IsClosing()) {
		simp::Begin();
		frame.DrawToBatch(simp::Batch(), { 0, 0 }, 0.f, { 4.f, 4.f });
		simp::End();
	}
}

void RunBubbles()
{
	auto& opts = opt::Get();
	simp::Init({ 1280, 720 }, L"Bubbly", *opts.logLevel);
	gfx::SpriteFrame frame{ {1, 1}, {0, 0}, simp::LoadAtlas(L"bubbles.png") };
	while (!simp::Win().IsClosing()) {
		simp::Begin();
		frame.DrawToBatch(simp::Batch(), { 0, 0 });
		simp::End();
	}
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PSTR pCmdLine,
	int nCmdShow)
{
	try {
		// parse the command line 
		if (auto code = opt::Init()) {
			if (*code == 0) {
				MessageBoxA(nullptr, opt::GetDiagnostics().c_str(), "Command Line Help",
					MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND);
			}
			else {
				MessageBoxA(nullptr, opt::GetDiagnostics().c_str(), "Command Line Parse Error",
					MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND);
			}
			return *code;
		}
		auto& opts = opt::Get();
		if (*opts.runMode == RunMode::Normal) {
			RunNormal();
		}
		else if (*opts.runMode == RunMode::Simple) {
			RunSimp();
		}
		else if (*opts.runMode == RunMode::Blown) {
			RunBlown();
		}
		else if (*opts.runMode == RunMode::Bubbles) {
			RunBubbles();
		}
		else {
			chilog.error(L"Unhandled RunMode detected");
		}
	}
	catch (const std::exception& e) {
		chilog.error(L"Error caught at top level: " + utl::ToWide(e.what())).no_trace();
		return -1;
	}

	return 0;
}