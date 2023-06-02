#include "RenderPane.h"
#include <Core/src/utl/HrChecker.h>
#pragma warning(push)
#pragma warning(disable : 26495)
#include "d3dx12.h"
#pragma warning(pop)

namespace chil::gfx::d12
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;

	RenderPane::RenderPane(HWND hWnd, const spa::DimensionsI& dims, std::shared_ptr<IDevice> pDevice,
		std::shared_ptr<ICommandQueue> pCommandQueue)
		:
		pDevice_{ std::move(pDevice) },
		pCommandQueue_{ std::move(pCommandQueue) }
	{
		// cache device interface
		auto pDeviceInterface = pDevice_->GetD3D12DeviceInterface();
		// create swap chain
		{
			const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
				.Width = (UINT)dims.width,
				.Height = (UINT)dims.height,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.Stereo = FALSE,
				.SampleDesc = {
					.Count = 1,
					.Quality = 0
				},
				.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
				.BufferCount = bufferCount_,
				.Scaling = DXGI_SCALING_STRETCH,
				.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
				.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
				.Flags = 0,
			};
			ComPtr<IDXGISwapChain1> swapChain1;
			pDevice_->GetDXGIFactoryInterface()->CreateSwapChainForHwnd(
				pCommandQueue_->GetD3D12CommandQueue().Get(),
				hWnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain1) >> chk;
			swapChain1.As(&pSwapChain_) >> chk;
		}
		// create descriptor heap
		{
			const D3D12_DESCRIPTOR_HEAP_DESC desc = {
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
				.NumDescriptors = bufferCount_,
			};
			pDeviceInterface->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pRtvDescriptorHeap_)) >> chk;
		}
		// cache descriptor size
		rtvDescriptorSize_ = pDeviceInterface->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		// create rtvs and get resource handles for each buffer in the swap chain
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
				pRtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
			for (int i = 0; i < bufferCount_; i++) {
				pSwapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i])) >> chk;
				pDeviceInterface->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(rtvDescriptorSize_);
			}
		}
	}

	RenderPane::~RenderPane()
	{
		// wait for queue to become completely empty
		pCommandQueue_->Flush();
	}

	void RenderPane::BeginFrame()
	{
		// set index of swap chain buffer for this frame
		curBackBufferIndex_ = pSwapChain_->GetCurrentBackBufferIndex();
		// wait for this back buffer to become free
		pCommandQueue_->WaitForFenceValue(bufferFenceValues_[curBackBufferIndex_]);
		// acquire command list
		commandListPair_ = pCommandQueue_->GetCommandListPair();
		// transition buffer resource to render target state 
		auto& backBuffer = backBuffers_[curBackBufferIndex_];
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandListPair_.pCommandList->ResourceBarrier(1, &barrier);
	}

	void RenderPane::EndFrame()
	{
		auto& backBuffer = backBuffers_[curBackBufferIndex_];
		// prepare buffer for presentation by transitioning to present state
		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				backBuffer.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			commandListPair_.pCommandList->ResourceBarrier(1, &barrier);
		}
		// submit command list 
		pCommandQueue_->ExecuteCommandList(std::move(commandListPair_));
		// present frame 
		pSwapChain_->Present(1, 0) >> chk;
		// insert a fence so we know when the buffer is free
		bufferFenceValues_[curBackBufferIndex_] = pCommandQueue_->SignalFence();
	}

	void RenderPane::Clear(const std::array<float, 4>& color)
	{
		auto& backBuffer = backBuffers_[curBackBufferIndex_];
		const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv{
			pRtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
			(INT)curBackBufferIndex_, rtvDescriptorSize_ };
		commandListPair_.pCommandList->ClearRenderTargetView(rtv, color.data(), 0, nullptr);
	}
}
