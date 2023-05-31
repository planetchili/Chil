#pragma once
#include "../IRenderPane.h"
#include "Device.h"
#include <Core/src/win/IWindow.h>
#include <wrl/client.h>
#include <d3d11_4.h>
#include <array>

namespace chil::gfx::d11
{
	class IRenderPane : public gfx::IRenderPane
	{
	public:
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Clear(const std::array<float, 4>& color) = 0;
	};

	class RenderPane : public IRenderPane
	{
	public:
		RenderPane(HWND hWnd, const spa::DimensionsI& dims, std::shared_ptr<IDevice> pDevice);
		void BeginFrame() override;
		void EndFrame() override;
		void Clear(const std::array<float, 4>& color) override;
	private:
		std::shared_ptr<IDevice> pDevice_;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> pDeferredContext_;
		Microsoft::WRL::ComPtr<IDXGISwapChain1> pSwapChain_;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pTargetView_;
		Microsoft::WRL::ComPtr<ID3D11CommandList> pCommandList_;
	};
}