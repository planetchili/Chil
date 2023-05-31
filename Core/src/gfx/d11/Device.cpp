#include "Device.h"
#include <Core/src/utl/HrChecker.h>
#include <Core/src/log/Log.h>

namespace chil::gfx::d11
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;

	Device::Device()
		:
		commandListExecutor_{ &Device::CommandListExecutorKernel_, this }
	{
		UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifndef NDEBUG
		createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createFlags,
			nullptr, 0,
			D3D11_SDK_VERSION,
			&pDevice_,
			nullptr,
			&pImmediateContext_
		) >> chk;
	}

	void Device::CommandListExecutorKernel_()
	{
		auto st = commandListExecutor_.get_stop_token();
		std::unique_lock lk{ commandListMutex_ };
		while (commandListCondition_.wait(lk, st, [this] {return !commandListQueue_.empty(); })) {
			auto pCommands = std::move(commandListQueue_.front());
			commandListQueue_.pop_front();
			pImmediateContext_->ExecuteCommandList(pCommands.Get(), FALSE);
		}
	}

	void Device::Execute(ComPtr<ID3D11CommandList> pCommandList)
	{
		{
			std::lock_guard lk{ commandListMutex_ };
			commandListQueue_.push_back(std::move(pCommandList));
		}
		commandListCondition_.notify_one();
	}

	ComPtr<ID3D11DeviceContext> Device::CreateDeferredContext()
	{
		ComPtr<ID3D11DeviceContext> pContext;
		pDevice_->CreateDeferredContext(0, &pContext) >> chk;
		return pContext;
	}

	ComPtr<IDXGISwapChain1> Device::CreateSwapChain(HWND hWnd, spa::DimensionsI size)
	{
		ComPtr<IDXGIDevice> pDxgiDevice;
		pDevice_.As(&pDxgiDevice) >> chk;

		UINT createFlags = 0;
#ifndef NDEBUG
		createFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
		ComPtr<IDXGIFactory2> pDxgiFactory;
		CreateDXGIFactory2(
			createFlags,
			IID_PPV_ARGS(&pDxgiFactory)
		) >> chk;

		// set swap chain configuration
		const DXGI_SWAP_CHAIN_DESC1 description{
			.Width = (UINT)size.width,
			.Height = (UINT)size.height,
			.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
			.SampleDesc = {.Count = 1 },
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = 2,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		};
		ComPtr<IDXGISwapChain1> pSwapChain;
		pDxgiFactory->CreateSwapChainForHwnd(
			pDxgiDevice.Get(),
			hWnd,
			&description,
			nullptr,
			nullptr,
			&pSwapChain
		) >> chk;

		return pSwapChain;
	}

	ComPtr<ID3D11RenderTargetView> Device::CreateRenderTargetView(ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc)
	{
		ComPtr<ID3D11RenderTargetView> pRenderTargetView;
		pDevice_->CreateRenderTargetView(pResource, pDesc, &pRenderTargetView) >> chk;
		return pRenderTargetView;
	}

	std::unique_lock<std::mutex> Device::LockMeDaddy()
	{
		return std::unique_lock{ commandListMutex_ };
	}
}