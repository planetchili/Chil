#include <Core/src/win/ChilWin.h>
#include <Core/src/ioc/Container.h> 
#include <Core/src/log/SeverityLevelPolicy.h> 
#include <Core/src/win/Boot.h>
#include <Core/src/log/Log.h> 
#include <Core/src/win/IWindow.h>
#include <Core/src/gfx/d11/Device.h>
#include <Core/src/gfx/d11/RenderPane.h>
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
	Boot();

	class ActiveWindow
	{
	public:
		ActiveWindow(std::shared_ptr<gfx::d11::Device> pDevice)
			:
			thread_{ &ActiveWindow::Kernel_, this, std::move(pDevice) }
		{
			constructionSemaphore_.acquire();
		}
		bool IsLive() const
		{
			return isLive;
		}
	private:
		void Kernel_(std::shared_ptr<gfx::d11::Device> pDevice)
		{
			// do construction
			std::shared_ptr<win::IWindow> pWindow_ = ioc::Get().Resolve<win::IWindow>();
			std::shared_ptr<gfx::d11::IRenderPane> pPane_ = std::make_shared<gfx::d11::RenderPane>(
				pWindow_->GetHandle(),
				spa::DimensionsI{ 1280, 720 },
				std::move(pDevice)
			);
			constructionSemaphore_.release();

			// do render loop while window not closing
			float c = 0;
			while (!pWindow_->IsClosing()) {
				pPane_->BeginFrame();
				pPane_->Clear({ c, c, c, 1.f });
				pPane_->EndFrame();
			}
		}
		std::binary_semaphore constructionSemaphore_{ 0 };
		std::thread thread_;
		std::atomic<bool> isLive{ true };
	};

	auto pDevice = std::make_shared<gfx::d11::Device>();

	std::vector<std::unique_ptr<ActiveWindow>> windows;
	for (size_t i = 0; i < 2; i++) {
		windows.push_back(std::make_unique<ActiveWindow>(pDevice));
	}

	float c = 0;
	while (!windows.empty()) {
		std::erase_if(windows, [](auto& p) {return !p->IsLive(); });
		std::this_thread::sleep_for(50ms);
	}

	return 0;
}