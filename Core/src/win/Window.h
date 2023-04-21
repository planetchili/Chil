#pragma once 
#include "IWindow.h" 
#include "WindowClass.h" 
#include <string> 
#include <thread> 
#include <semaphore> 
#include <atomic> 
#include <optional>
#include <Core/src/spa/Dimensions.h> 
#include <Core/src/spa/Vec2.h>
#include <Core/src/ccr/GenericTaskQueue.h> 

namespace chil::win
{
	class Window : public IWindow
	{
	public:
		Window(std::shared_ptr<IWindowClass> pWindowClass, std::wstring title,
			spa::DimensionsI clientAreaSize, std::optional<spa::Vec2I> position = {});
		HWND GetHandle() const override;
		bool IsClosing() const override;
		~Window() override;
	protected:
		// constants 
		static constexpr UINT CustomTaskMessageId = WM_USER + 0;
		// functions
		virtual void MessageKernel_() noexcept;
		LRESULT HandleMessage_(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept override;
		template<class F>
		auto Dispatch_(F&& f)
		{
			auto future = tasks_.Push(std::forward<F>(f));
			NotifyTaskDispatch_();
			return future;
		}
		void NotifyTaskDispatch_();
		// data 
		mutable ccr::GenericTaskQueue tasks_;
		std::binary_semaphore startSignal_{ 0 };
		std::thread kernelThread_;
		HWND hWnd_ = nullptr;
		std::atomic<bool> closing_ = false;
	};
}