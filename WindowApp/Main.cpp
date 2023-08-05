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
		ActiveWindow(std::shared_ptr<gfx::d12::Device> pDevice, std::shared_ptr<gfx::d12::ITexture> pTexture)
			:
			thread_{ &ActiveWindow::Kernel_, this, std::move(pDevice), std::move(pTexture) }
		{
			constructionSemaphore_.acquire();
		}
		bool IsLive() const
		{
			return isLive;
		}
	private:
		// functions
		void Kernel_(std::shared_ptr<gfx::d12::Device> pDevice, std::shared_ptr<gfx::d12::ITexture> pTexture)
		{
			//// do construction
			// make window
			std::shared_ptr<win::IWindow> pWindow_ = ioc::Get().Resolve<win::IWindow>();
			// make graphics pane
			std::shared_ptr<gfx::d12::IRenderPane> pPane_ = std::make_shared<gfx::d12::RenderPane>(
				pWindow_->GetHandle(),
				spa::DimensionsI{ 1280, 720 },
				pDevice,
				std::make_shared<gfx::d12::CommandQueue>(pDevice)
			);
			// make sprite batcher
			gfx::d12::SpriteBatcher batcher{ pDevice };
			// add sprite sheet to batcher
			batcher.AddTexture(pTexture);
			// signal completion of construction phase
			constructionSemaphore_.release();

			// character
			class Character
			{
			public:
				Character(spa::Vec2F center, float radius, float period, float phase)
					:
					center_{ center },
					radius_{ radius },
					period_{ period },
					phase_{ phase }
				{}
				void Draw(gfx::d12::SpriteBatcher& batcher, float t) const
				{
					const auto theta = t * period_ / (2.f * std::numbers::pi_v<float>) + phase_;
					const auto pos = center_ + spa::Vec2F{ std::cos(theta), std::sin(theta) } * radius_;
					batcher.Draw(
						spa::RectF::FromPointAndDimensions({ 0.f, 0.f }, { 1.f / 8.f, 1.f / 4.f }),
						spa::RectF::FromPointAndDimensions(pos, { .2, -.4 })
					);
				}
			private:
				spa::Vec2F center_;
				float radius_;
				float period_;
				float phase_;
			};
			// frame variables
			float t = 0.f;
			const auto characters =
				vi::iota(0, 2000) |
				vi::transform([
					rne = std::minstd_rand0{ std::random_device{}() },
					posDist = std::uniform_real_distribution<float>{ -1.f, 1.f },
					radDist = std::uniform_real_distribution<float>{ 0.f, .4f },
					perDist = std::uniform_real_distribution<float>{ 1.f, 20.f },
					phaDist = std::uniform_real_distribution<float>{ 0.f, 2.f * std::numbers::pi_v<float> }
				] (auto) mutable -> Character {
					return { { posDist(rne), posDist(rne) }, radDist(rne), perDist(rne), phaDist(rne) };
				}) |
				rn::to<std::vector>();
			// do render loop while window not closing
			while (!pWindow_->IsClosing()) {
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
				t += 0.01f;
			}
			pPane_->FlushQueues();

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
		// load texture
		gfx::d12::ResourceLoader loader{ pDevice };
		auto pTexture = loader.LoadTexture(L"sprote-shiet.png").get();

		std::vector<std::unique_ptr<ActiveWindow>> windows;
		for (size_t i = 0; i < 6; i++) {
			windows.push_back(std::make_unique<ActiveWindow>(pDevice, pTexture));
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