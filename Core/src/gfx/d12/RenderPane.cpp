#include "RenderPane.h"
#include <Core/src/utl/HrChecker.h>

namespace chil::gfx::d12
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;

	RenderPane::RenderPane(HWND hWnd, const spa::DimensionsI& dims, std::shared_ptr<IDevice> pDevice)
		:
		pDevice_{ std::move(pDevice) }
	{
		auto device = pDevice_->GetD3D12DeviceInterface();
		{
			const D3D12_COMMAND_QUEUE_DESC desc = {
				.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
				.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0,
			};
			device->CreateCommandQueue(&desc, IID_PPV_ARGS(&pCommandQueue_)) >> chk;
		}
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&pCommandAllocator_)) >> chk;
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			pCommandAllocator_.Get(), nullptr, IID_PPV_ARGS(&pCommandList_)) >> chk;
		// initially close the command list so it can be reset at top of draw loop 
		pCommandList_->Close() >> chk;
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence_)) >> chk;
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
				pCommandQueue_.Get(),
				hWnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain1) >> chk;
			swapChain1.As(&pSwapChain_) >> chk;
		}
		{
			const D3D12_DESCRIPTOR_HEAP_DESC desc = {
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
				.NumDescriptors = bufferCount_,
			};
			device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pRtvDescriptorHeap_)) >> chk;
		}
		rtvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
				pRtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
			for (int i = 0; i < bufferCount_; i++) {
				pSwapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i])) >> chk;
				device->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(rtvDescriptorSize_);
			}
		}
	}

	RenderPane::~RenderPane()
	{
		// wait for queue to become completely empty
		pCommandQueue_->Signal(pFence_.Get(), ++fenceValue_) >> chk;
		pFence_->SetEventOnCompletion(fenceValue_, nullptr) >> chk;
	}

	void RenderPane::BeginFrame()
	{
		curBackBufferIndex_ = pSwapChain_->GetCurrentBackBufferIndex();
		// reset command list and allocator 
		pCommandAllocator_->Reset() >> chk;
		pCommandList_->Reset(pCommandAllocator_.Get(), nullptr) >> chk;
	}

	void RenderPane::EndFrame()
	{
		auto& backBuffer = backBuffers_[curBackBufferIndex_];
		// prepare buffer for presentation by transitioning to present state
		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				backBuffer.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			pCommandList_->ResourceBarrier(1, &barrier);
		}
		// submit command list 
		{
			// close command list 
			pCommandList_->Close() >> chk;
			// submit command list to queue as array with single element
			ID3D12CommandList* const commandLists[] = { pCommandList_.Get() };
			pCommandQueue_->ExecuteCommandLists((UINT)std::size(commandLists), commandLists);
		}
		// insert fence to mark command list completion 
		pCommandQueue_->Signal(pFence_.Get(), ++fenceValue_) >> chk;
		// present frame 
		pSwapChain_->Present(1, 0) >> chk;
		// wait for command list / allocator to become free 
		pFence_->SetEventOnCompletion(fenceValue_, nullptr) >> chk;
	}

	void RenderPane::Clear(const std::array<float, 4>& color)
	{
		auto& backBuffer = backBuffers_[curBackBufferIndex_];
		// transition buffer resource to render target state 
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pCommandList_->ResourceBarrier(1, &barrier);
		// clear rtv 
		const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv{
			pRtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
			(INT)curBackBufferIndex_, rtvDescriptorSize_ };
		pCommandList_->ClearRenderTargetView(rtv, color.data(), 0, nullptr);
	}
}
