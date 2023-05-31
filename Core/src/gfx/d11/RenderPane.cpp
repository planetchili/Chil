#include "RenderPane.h"
#include <Core/src/utl/HrChecker.h>

namespace chil::gfx::d11
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;

	RenderPane::RenderPane(HWND hWnd, const spa::DimensionsI& dims, std::shared_ptr<IDevice> pDevice)
		:
		pDevice_{ std::move(pDevice) }
	{
		pSwapChain_ = pDevice_->CreateSwapChain(hWnd, dims);
		{
			ComPtr<ID3D11Texture2D> pBackBuffer;
			pSwapChain_->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)) >> chk;
			pTargetView_ = pDevice_->CreateRenderTargetView(pBackBuffer.Get(), nullptr);
		}
		pDeferredContext_ = pDevice_->CreateDeferredContext();
	}

	void RenderPane::BeginFrame()
	{
		pDeferredContext_->ClearState();
		pDeferredContext_->OMSetRenderTargets(1, pTargetView_.GetAddressOf(), nullptr);
	}

	void RenderPane::EndFrame()
	{
		ComPtr<ID3D11CommandList> pCommandList;
		pDeferredContext_->FinishCommandList(FALSE, &pCommandList) >> chk;
		pDevice_->Execute(std::move(pCommandList));
		pSwapChain_->Present(1, 0);
	}

	void RenderPane::Clear(const std::array<float, 4>& color)
	{
		pDeferredContext_->ClearRenderTargetView(pTargetView_.Get(), color.data());
	}
}
