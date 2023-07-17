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

			// do render loop while window not closing
			while (!pWindow_->IsClosing()) {
				pPane_->BeginFrame();
				batcher.StartBatch(
					pPane_->GetCommandList(),
					pPane_->GetFrameFenceValue(),
					pPane_->GetSignalledFenceValue()
				);
				batcher.Draw(
					{ .left = 0.f, .top = 0.f, .right = 1.f, .bottom = 1.f },
					spa::RectF::FromPointAndDimensions({-.6f, 0.f}, {.2, .4})
				);
				pPane_->SubmitCommandList(batcher.EndBatch());
				pPane_->EndFrame();
			}

			isLive = false;
		}
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
		for (size_t i = 0; i < 1; i++) {
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