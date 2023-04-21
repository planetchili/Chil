#pragma once 
#include "ChilWin.h"
#include <future>
#include <string>

namespace chil::win
{
	class IWindow
	{
		// allow WindowClasses access to the message handling function 
		friend class IWindowClass;
	public:
		virtual ~IWindow() = default;
		virtual HWND GetHandle() const = 0;
		virtual bool IsClosing() const = 0;
		virtual std::future<void> SetTitle(std::wstring title) = 0;
	protected:
		virtual LRESULT HandleMessage_(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept = 0;
	};
}