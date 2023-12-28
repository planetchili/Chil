#include <Core/src/win/ChilWin.h>
#include <Core/src/ioc/Container.h> 
#include <Core/src/log/SeverityLevelPolicy.h> 
#include <Core/src/win/Boot.h>
#include <Core/src/log/Log.h> 
#include <Core/src/win/IWindow.h>
#include <Core/src/gfx/d12/Device.h>
#include <Core/src/gfx/d12/RenderPane.h>
#include <Core/src/gfx/d12/CommandQueue.h>
#include <Core/src/gfx/d12/ResourceLoader.h>
#include <Core/src/gfx/d12/SpriteBatcher.h>
#include "Sprite.h"
#include <Core/src/win/Input.h>
#include <format>
#include <ranges> 
#include <semaphore>
#include <numbers>
#include <random>

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

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PWSTR pCmdLine,
	int nCmdShow)
{
	class ActiveWindow
	{
	public:
		ActiveWindow(int index,
			std::shared_ptr<gfx::d12::Device> pDevice,
			std::shared_ptr<gfx::d12::SpriteCodex> pSpriteCodex)
			:
			thread_{ &ActiveWindow::Kernel_, this, index, std::move(pDevice), std::move(pSpriteCodex) }
		{
			constructionSemaphore_.acquire();
		}
		bool IsLive() const
		{
			return isLive;
		}
	private:
		// functions
		void Kernel_(int index,
			std::shared_ptr<gfx::d12::Device> pDevice,
			std::shared_ptr<gfx::d12::SpriteCodex> pSpriteCodex)
		{
#ifdef NDEBUG
			const unsigned int nCharacters = 250'000;
#else
			const unsigned int nCharacters = 50;
#endif
			const auto outputDims = spa::DimensionsI{ 1280, 720 };
			//// do construction
			// make window
			auto keyboard = std::make_shared<win::Keyboard>();
			std::shared_ptr<win::IWindow> pWindow_ = ioc::Get().Resolve<win::IWindow>(
				win::IWindow::IocParams{
					.pKeySink = keyboard,
					.name = std::format(L"Window #{}", index),
				}
			);
			// make graphics pane
			std::shared_ptr<gfx::d12::IRenderPane> pPane_ = std::make_shared<gfx::d12::RenderPane>(
				pWindow_->GetHandle(),
				outputDims,
				pDevice,
				std::make_shared<gfx::d12::CommandQueue>(pDevice)
			);
			// make sprite batcher
			gfx::d12::SpriteBatcher batcherConcrete{ outputDims, pDevice, pSpriteCodex, nCharacters };
			gfx::d12::VINTERFACE(SpriteBatcher)& batcher = batcherConcrete;
			// signal completion of construction phase
			constructionSemaphore_.release();

			// random engine
			std::minstd_rand0 rne;
			// sprite blueprints
			const auto pBlueprint = std::make_shared<SpriteBlueprint>(pSpriteCodex, 0, 8, 4);
			// sprite instances
			const auto characters =
				vi::iota(0u, nCharacters) |
				vi::transform([
					pBlueprint,
					rne,
					posDist = std::uniform_real_distribution<float>{ -360.f, 360.f },
					speedDist = std::uniform_real_distribution<float>{ 240.f, 600.f },
					angleDist = std::uniform_real_distribution<float>{ 0.f, 2.f * std::numbers::pi_v<float> }
				] (auto) mutable -> std::unique_ptr<VINTERFACE(SpriteInstance)> {
					return std::make_unique<SpriteInstance>(pBlueprint,
						spa::Vec2F{ posDist(rne), posDist(rne) },
						spa::Vec2F{ 1.f, 0.f }.GetRotated(angleDist(rne)) * speedDist(rne)
					);
				}) |
				rn::to<std::vector>();

			// camera state variables
			spa::Vec2F pos{};
			float rot = 0.f;
			float scale = 1.f;

			// do render loop while window not closing
			while (!pWindow_->IsClosing()) {
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
				batcher.SetCamera(pos, rot, scale);

				// update sprites
				for (const auto& pc : characters) {
					pc->Update(0.001f, rne);
				}

				// render frame
				pPane_->BeginFrame();
				batcher.StartBatch(
					pPane_->GetCommandList(),
					pPane_->GetFrameFenceValue(),
					pPane_->GetSignalledFenceValue()
				);
				for (const auto& pc : characters) {
					pc->Draw(batcher);
				}
				pPane_->SubmitCommandList(batcher.EndBatch());
				pPane_->EndFrame();
			}
			pPane_->FlushQueues();

			chilog.info(std::format(L"sprites: {}", characters.size()));

			isLive = false;
		}
		// data
		std::binary_semaphore constructionSemaphore_{ 0 };
		std::atomic<bool> isLive{ true };
		std::jthread thread_;
	};

	try {
		// init COM
		if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
			throw std::runtime_error{ "COM farked" };
		}

		Boot();

		// create device
		auto pDevice = std::make_shared<gfx::d12::Device>();
		// create sprite codex
		auto pSpriteCodex = std::make_shared<gfx::d12::SpriteCodex>(pDevice);
		// load texture into sprite codex
		gfx::d12::ResourceLoader loader{ pDevice };
		pSpriteCodex->AddSpriteAtlas(loader.LoadTexture(L"sprote-shiet-bak.png").get());

		std::vector<std::unique_ptr<ActiveWindow>> windows;
		for (int i = 0; i < 1; i++) {
			windows.push_back(std::make_unique<ActiveWindow>(i, pDevice, pSpriteCodex));
		}

		float c = 0;
		while (!windows.empty()) {
			std::erase_if(windows, [](auto& p) {return !p->IsLive(); });
			std::this_thread::sleep_for(50ms);
		}
	}
	catch (const std::exception& e) {
		chilog.error(utl::ToWide(e.what())).no_trace();
		return -1;
	}

	return 0;
}