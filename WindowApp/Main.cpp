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
			std::shared_ptr<gfx::d12::SpriteCodex> pSpriteCodex,
			std::shared_ptr<gfx::d12::SpriteFrame> pFrame,
			std::shared_ptr<gfx::d12::SpriteFrame> pFrame2)
			:
			thread_{ &ActiveWindow::Kernel_, this, index, std::move(pDevice), std::move(pSpriteCodex), std::move(pFrame), std::move(pFrame2) }
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
			std::shared_ptr<gfx::d12::SpriteCodex> pSpriteCodex,
			std::shared_ptr<gfx::d12::SpriteFrame> pFrame,
			std::shared_ptr<gfx::d12::SpriteFrame> pFrame2)
		{
			const unsigned int nCharacters = 4;
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
			gfx::d12::SpriteBatcher batcher{ outputDims, pDevice, std::move(pSpriteCodex), nCharacters };
			// signal completion of construction phase
			constructionSemaphore_.release();

			// character
			class Character
			{
			public:
				Character(std::shared_ptr<gfx::d12::SpriteFrame> pSpriteFrame,
					spa::Vec2F center, float radius,
					float periodOrbit, float phaseOrbit,
					float periodRotate, float phaseRotate
					)
					:
					pSpriteFrame_{ std::move(pSpriteFrame) },
					center_{ center },
					radius_{ radius },
					periodOrbit_{ periodOrbit },
					phaseOrbit_{ phaseOrbit },
					periodRotate_{ periodRotate },
					phaseRotate_{ phaseRotate }
				{}
				void Draw(gfx::d12::SpriteBatcher& batcher, float t) const
				{
					const auto thetaOrbit = t * periodOrbit_ / (2.f * std::numbers::pi_v<float>) + phaseOrbit_;
					const auto thetaRotate = t * periodRotate_ / (2.f * std::numbers::pi_v<float>) + phaseRotate_;
					const auto pos = center_ + spa::Vec2F{ std::cos(thetaOrbit), std::sin(thetaOrbit) } * radius_;
					pSpriteFrame_->DrawToBatch(batcher, pos, thetaRotate);
				}
			private:
				std::shared_ptr<gfx::d12::SpriteFrame> pSpriteFrame_;
				spa::Vec2F center_;
				float radius_;
				float periodOrbit_;
				float phaseOrbit_;
				float periodRotate_;
				float phaseRotate_;
			};
			// frame variables
			float t = 0.f;
			//std::vector<Character> characters;
			const auto characters =
				vi::iota(0u, nCharacters) |
				vi::transform([
					pFrame,
					rne = std::minstd_rand0{ 0 },
					posDist = std::uniform_real_distribution<float>{ -360.f, 360.f },
					radDist = std::uniform_real_distribution<float>{ 0.f, 200.f },
					perDist = std::uniform_real_distribution<float>{ 1.f, 20.f },
					phaDist = std::uniform_real_distribution<float>{ 0.f, 2.f * std::numbers::pi_v<float> },
					perDist2 = std::uniform_real_distribution<float>{ 1.f, 20.f },
					phaDist2 = std::uniform_real_distribution<float>{ 0.f, 2.f * std::numbers::pi_v<float> }
				] (auto) mutable -> Character {
					return { pFrame, { posDist(rne), posDist(rne) }, radDist(rne), perDist(rne), phaDist(rne), perDist2(rne), phaDist2(rne) };
				}) |
				rn::to<std::vector>();
			// do render loop while window not closing
			spa::Vec2F pos{};
			float rot = 0.f;
			float scale = 1.f;
			while (!pWindow_->IsClosing()) {
				while (const auto e = keyboard->GetEvent()) {
					if (e->type == win::KeyEvent::Type::Release) continue;
					switch (e->code) {
					case VK_SPACE:
						//[
						//	&characters,
						//		pFrame,
						//		rne = std::minstd_rand0{ std::random_device{}() },
						//		posDist = std::uniform_real_distribution<float>{ -360.f, 360.f },
						//		radDist = std::uniform_real_distribution<float>{ 0.f, 200.f },
						//		perDist = std::uniform_real_distribution<float>{ 1.f, 20.f },
						//		phaDist = std::uniform_real_distribution<float>{ 0.f, 2.f * std::numbers::pi_v<float> },
						//		perDist2 = std::uniform_real_distribution<float>{ 1.f, 20.f },
						//		phaDist2 = std::uniform_real_distribution<float>{ 0.f, 2.f * std::numbers::pi_v<float> }
						//] () mutable {
						//	characters.push_back(Character{
						//		pFrame, { posDist(rne), posDist(rne) },
						//		radDist(rne), perDist(rne), phaDist(rne),
						//		perDist2(rne), phaDist2(rne)
						//	});
						//}();
						//break;
						break;
					}
				}
				if (keyboard->KeyIsPressed('W')) {
					pos.y -= 1.f;
				}
				if (keyboard->KeyIsPressed('S')) {
					pos.y += 1.f;
				}
				if (keyboard->KeyIsPressed('D')) {
					pos.x -= 1.f;
				}
				if (keyboard->KeyIsPressed('A')) {
					pos.x += 1.f;
				}
				if (keyboard->KeyIsPressed('Q')) {
					rot += 0.02f;
				}
				if (keyboard->KeyIsPressed('E')) {
					rot -= 0.02f;
				}
				if (keyboard->KeyIsPressed('R')) {
					scale *= 1.02f;
				}
				if (keyboard->KeyIsPressed('F')) {
					scale /= 1.02f;
				}
				batcher.SetCamera(pos, rot, scale);
				pPane_->BeginFrame();
				batcher.StartBatch(
					pPane_->GetCommandList(),
					pPane_->GetFrameFenceValue(),
					pPane_->GetSignalledFenceValue()
				);
				for (const auto& c : characters) {
					c.Draw(batcher, t);
				}
				pPane_->SubmitCommandList(batcher.EndBatch());
				pPane_->EndFrame();
				// update time
				// t += 0.01f;
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
		pSpriteCodex->AddSpriteAtlas(loader.LoadTexture(L"sprote-shiet.png").get());
		// create sprite frame
		auto pSpriteFrame = std::make_shared<gfx::d12::SpriteFrame>(
			spa::DimensionsI{ 8, 4 }, spa::Vec2I{ 3, 1 }, 0, pSpriteCodex
		);
		auto pSpriteFrame2 = std::make_shared<gfx::d12::SpriteFrame>(
			spa::RectF::FromPointAndDimensions({ 0, 0 }, { 100, 200 }), 1, pSpriteCodex
		); 

		std::vector<std::unique_ptr<ActiveWindow>> windows;
		for (int i = 0; i < 1; i++) {
			windows.push_back(std::make_unique<ActiveWindow>(i, pDevice, pSpriteCodex, pSpriteFrame, pSpriteFrame2));
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