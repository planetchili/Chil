#include "Window.h" 
#include "Utilities.h" 
#include "Exception.h" 
#include <format> 
#include <Core/src/log/Log.h> 
#include <Core/src/utl/String.h> 

namespace chil::win
{
	Window::Window(std::shared_ptr<IWindowClass> pWindowClass, std::wstring title,
		spa::DimensionsI clientAreaSize, std::optional<spa::Vec2I> position)
		:
		pWindowClass_{ std::move(pWindowClass) },
		kernelThread_{ &Window::MessageKernel_, this }
	{
		auto future = tasks_.Push([=, this] {
			const DWORD styles = WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
			const DWORD exStyles = 0;
			const auto windowDims = ClientToWindowDimensions(clientAreaSize, styles);
			const auto hModule = GetModuleHandleW(nullptr);
			if (!hModule) {
				chilog.error().hr();
				throw WindowException{ "Failed to get module handle" };
			}
			hWnd_ = CreateWindowExW(
				exStyles,
				MAKEINTATOM(pWindowClass_->GetAtom()),
				title.c_str(),
				styles,
				position.transform([](auto v){return v.x;}).value_or(CW_USEDEFAULT),
				position.transform([](auto v){return v.y;}).value_or(CW_USEDEFAULT),
				windowDims.width, windowDims.height,
				nullptr, nullptr, hModule,
				this
			);
			if (!hWnd_) {
				chilog.error(L"Failed creating window").hr();
				throw WindowException{ "Failed creating window" };
			}
		});
		startSignal_.release();
		future.get();
	}
	HWND Window::GetHandle() const
	{
		return hWnd_;
	}
	bool Window::IsClosing() const
	{
		return closing_;
	}
	std::future<void> Window::SetTitle(std::wstring title)
	{
		return Dispatch_([=, this] {
			if (!SetWindowTextW(hWnd_, title.c_str())) {
				chilog.warn().hr();
			}
		});
	}
	Window::~Window()
	{
		Dispatch_([this] {
			if (!DestroyWindow(hWnd_)) {
				chilog.warn(L"Failed destroying window").hr();
			}
		});
		kernelThread_.join();
	}
	LRESULT Window::HandleMessage_(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		try {
			switch (msg) {
			case WM_DESTROY:
				hWnd_ = nullptr;
				PostQuitMessage(0);
				return 0;
			case WM_CLOSE:
				closing_ = true;
				return 0;
			case CustomTaskMessageId:
				tasks_.PopExecute();
				return 0;
			}
		}
		catch (const std::exception& e) {
			chilog.error(std::format(
				L"Uncaught exception in Windows message handler: {}",
				utl::ToWide(e.what())
			));
		}
		catch (...) {
			chilog.error(L"Uncaught annonymous exception in Windows message handler");
		}
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
	void Window::NotifyTaskDispatch_()
	{
		if (!PostMessageW(hWnd_, CustomTaskMessageId, 0, 0)) {
			chilog.error().hr();
			throw WindowException{ "Failed to post task notification message" };
		}
	}
	void Window::MessageKernel_() noexcept
	{
		startSignal_.acquire();
		tasks_.PopExecute();

		MSG msg{};
		while (GetMessageW(&msg, hWnd_, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
}