#include <Core/src/win/ChilWin.h>
#include <Core/src/ioc/Container.h> 
#include <Core/src/log/SeverityLevelPolicy.h> 
#include <Core/src/win/Boot.h>
#include <Core/src/log/Log.h> 
#include <Core/src/win/IWindow.h>
#include <Core/src/gfx/d12/Device.h>
#include <Core/src/gfx/d12/RenderPane.h>
#include <Core/src/gfx/d12/CommandQueue.h>
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
		ActiveWindow(std::shared_ptr<gfx::d12::Device> pDevice)
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
		void Kernel_(std::shared_ptr<gfx::d12::Device> pDevice)
		{
			// do construction
			std::shared_ptr<win::IWindow> pWindow_ = ioc::Get().Resolve<win::IWindow>();
			std::shared_ptr<gfx::d12::IRenderPane> pPane_ = std::make_shared<gfx::d12::RenderPane>(
				pWindow_->GetHandle(),
				spa::DimensionsI{ 1280, 720 },
				pDevice,
				std::make_shared<gfx::d12::CommandQueue>(pDevice)
			);
			constructionSemaphore_.release();

			// do render loop while window not closing
			float t = 0;
			while (!pWindow_->IsClosing()) {
				const std::array<float, 4> color{
					std::sin(t + .3f) * .5f + .5f,
					std::sin(1.1f * t + .2f) * .5f + .5f,
					std::sin(1.2f * t + .1f) * .5f + .5f,
					1.f
				};
				pPane_->BeginFrame();
				pPane_->Clear(color);
				pPane_->EndFrame();
				t += 0.01f;
			}

			isLive = false;
		}
		std::binary_semaphore constructionSemaphore_{ 0 };
		std::atomic<bool> isLive{ true };
		std::jthread thread_;
	};

	auto pDevice = std::make_shared<gfx::d12::Device>();

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