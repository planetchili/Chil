#include <Core/src/win/ChilWin.h>
#include <objbase.h>
#include <Core/src/ioc/Container.h> 
#include <Core/src/log/SeverityLevelPolicy.h> 
#include <Core/src/log/Log.h> 
#include <Core/src/win/Boot.h>
#include <Core/src/win/IWindow.h>
#include <Core/src/win/Input.h>
#include <Core/src/gfx/IRenderPane.h>
#include <Core/src/gfx/IResourceLoader.h>
#include <Core/src/gfx/ISpriteBatcher.h>
#include <Core/src/gfx/d12/Boot.h>
#include "Sprite.h"
#include <format>
#include <ranges> 
#include <semaphore>
#include <numbers>
#include <random>
#include <chrono>

using namespace chil;
using namespace std::string_literals;
using namespace std::chrono_literals;
using hrclock = std::chrono::high_resolution_clock;
namespace rn = std::ranges;
namespace vi = rn::views;


namespace Global
{
#ifdef NDEBUG
	constexpr unsigned int nCharacters = 250'000;
#else
	constexpr unsigned int nCharacters = 500;
#endif
	constexpr size_t nBatches = 4;
	constexpr int nSheets = 32;
	constexpr auto outputDims = spa::DimensionsI{ 1280, 720 };
	constexpr int nWindows = 1;
}


void Boot()
{
	log::Boot();
	ioc::Get().Register<log::ISeverityLevelPolicy>([] {
		return std::make_shared<log::SeverityLevelPolicy>(log::Level::Info);
	});

	win::Boot();

	gfx::d12::Boot();
}

class ActiveWindow
{
public:
	ActiveWindow(int index, std::shared_ptr<gfx::ISpriteCodex> pSpriteCodex)
		:
		thread_{ &ActiveWindow::Kernel_, this, index, std::move(pSpriteCodex) }
	{
		constructionSemaphore_.acquire();
		if (hasException_) {
			std::rethrow_exception(exception_);
		}
	}
	bool IsLive() const
	{
		if (hasException_) {
			std::rethrow_exception(exception_);
		}
		return isLive;
	}
private:
	// functions
	void Kernel_(int index, std::shared_ptr<gfx::ISpriteCodex> pSpriteCodex)
	{
		try {
			// ioc container shortcut
			auto& C = ioc::Get();
			//// do construction
			// make sprite batchers
			auto batchers = vi::iota(0ull, Global::nBatches) | vi::transform([&](auto i) {
				return C.Resolve<gfx::ISpriteBatcher>(gfx::ISpriteBatcher::IocParams{
					.targetDimensions = Global::outputDims,
					.pSpriteCodex = pSpriteCodex,
					.maxSpriteCount = UINT(Global::nCharacters / Global::nBatches + 1)
				});
			}) | rn::to<std::vector>();
			// make window
			auto keyboard = std::make_shared<win::Keyboard>();
			auto pWindow = C.Resolve<win::IWindow>(win::IWindow::IocParams{
				.pKeySink = keyboard,
				.name = std::format(L"Window #{}", index),
				.size = Global::outputDims,
				});
			// make graphics pane
			auto pPane = C.Resolve<gfx::IRenderPane>(gfx::IRenderPane::IocParams{
				.hWnd = pWindow->GetHandle(),
				.dims = Global::outputDims,
				});
			// signal completion of construction phase
			constructionSemaphore_.release();

			// random engine
			std::minstd_rand0 rne;
			// sprite blueprints
			std::vector<std::shared_ptr<ISpriteBlueprint>> blueprints;
			for (int i = 0; i < Global::nSheets; i++) {
				blueprints.push_back(std::make_shared<SpriteBlueprint>(pSpriteCodex, i, 8, 4));
			}
			// sprite instances
			const auto characters =
				vi::iota(0u, Global::nCharacters) |
				vi::transform([
					&blueprints,
						&rne,
						posDist = std::uniform_real_distribution<float>{ -360.f, 360.f },
						speedDist = std::uniform_real_distribution<float>{ 240.f, 600.f },
						angleDist = std::uniform_real_distribution<float>{ 0.f, 2.f * std::numbers::pi_v<float> }
				] (uint32_t i) mutable -> std::unique_ptr<ISpriteInstance> {
				return std::make_unique<SpriteInstance>(blueprints[i % Global::nSheets],
				spa::Vec2F{ posDist(rne), posDist(rne) },
				spa::Vec2F{ 1.f, 0.f }.GetRotated(angleDist(rne)) * speedDist(rne)
				);
			}) | rn::to<std::vector>();

			// camera state variables
			spa::Vec2F pos{};
			float rot = 0.f;
			float scale = 1.f;
			// pair batchers together with a set of characters to draw to each batch
			auto batches = vi::zip(batchers, characters | vi::chunk(characters.size() / batchers.size() + 1));

			// do render loop while window not closing
			while (!pWindow->IsClosing()) {
				// camera movement controls
				if (keyboard->KeyIsPressed('W')) {
					constexpr auto deg90 = float(std::numbers::pi / 2.);
					auto up = spa::Vec2F{ std::cos(rot + deg90), std::sin(rot + deg90) };
					pos += up * 10.f;
				}
				else if (keyboard->KeyIsPressed('S')) {
					constexpr auto deg90 = float(std::numbers::pi / 2.);
					auto up = spa::Vec2F{ std::cos(rot + deg90), std::sin(rot + deg90) };
					pos -= up * 10.f;
				}
				if (keyboard->KeyIsPressed('D')) {
					auto right = spa::Vec2F{ std::cos(rot), std::sin(rot) };
					pos += right * 10.f;
				}
				else if (keyboard->KeyIsPressed('A')) {
					auto right = spa::Vec2F{ std::cos(rot), std::sin(rot) };
					pos -= right * 10.f;
				}
				if (keyboard->KeyIsPressed('Q')) {
					rot += 0.02f;
				}
				else if (keyboard->KeyIsPressed('E')) {
					rot -= 0.02f;
				}
				if (keyboard->KeyIsPressed('R')) {
					scale *= 1.02f;
				}
				else if (keyboard->KeyIsPressed('F')) {
					scale /= 1.02f;
				}
				for (auto& pBatcher : batchers) {
					pBatcher->SetCamera(pos, rot, scale);
				}

				// update sprites
				const auto markStartSpriteUpdate = hrclock::now();
				{
					auto drawFutures = batches | vi::transform([&](auto&& batch) {
						return std::async([&](auto&& batch) {
							auto&& [pBatcher, spritePtrRange] = batch;
							for (const auto& ps : spritePtrRange) {
								ps->Update(0.001f, rne);
							}
						}, batch);
					}) | rn::to<std::vector>();
				}
				const auto durationSpriteUpdate = hrclock::now() - markStartSpriteUpdate;
				const auto spriteUpdateMs = std::chrono::duration<float, std::milli>(durationSpriteUpdate).count();

				// begin frame
				pPane->BeginFrame();
				// prepare each batcher to draw to
				for (auto& pBatcher : batchers) {
					pBatcher->StartBatch(*pPane);
				}
				// spawn a thread for each batch to draw sprites to batchers
				const auto markStartSpriteDraw = hrclock::now();
				{
					auto drawFutures = batches | vi::transform([&](auto&& batch) {
						return std::async([&](auto&& batch) {
							auto&& [pBatcher, spritePtrRange] = batch;
							for (const auto& ps : spritePtrRange) {
								ps->Draw(*pBatcher);
							}
						}, batch);
					}) | rn::to<std::vector>();
				}
				const auto durationSpriteDraw = hrclock::now() - markStartSpriteDraw;
				const auto spriteDrawMs = std::chrono::duration<float, std::milli>(durationSpriteDraw).count();
				// finish all batcher batches and submit resulting command list to the pane
				for (auto& pBatcher : batchers) {
					pBatcher->EndBatch(*pPane);
				}
				// finish and present the frame
				pPane->EndFrame();

				// output benching information
				OutputDebugStringA(std::format("s-up [{:>8.4f}ms]  s-draw [{:>8.4f}ms]\n", spriteUpdateMs, spriteDrawMs).c_str());
			}
			pPane->FlushQueues();
			isLive = false;
		}
		catch (...) {
			exception_ = std::current_exception();
			hasException_ = true;
			constructionSemaphore_.release();
		}
	}
	// data
	std::binary_semaphore constructionSemaphore_{ 0 };
	std::atomic<bool> isLive{ true };
	std::jthread thread_;
	std::atomic<bool> hasException_ = false;
	std::exception_ptr exception_;
};


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