#include "RenderPane.h"
#include <Core/src/utl/HrChecker.h>
#include <Core/src/log/Log.h>
#include <Core/src/utl/String.h>
#include "ResourceLoader.h"
#include "WrapD3DX.h"
#include "WrapDXGI.h"
#include <d3dcompiler.h>
#include <ranges>

namespace chil::gfx::d12
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;
	namespace rn = std::ranges;

	RenderPane::RenderPane(HWND hWnd, const spa::DimensionsI& dims, std::shared_ptr<IDevice> pDevice,
		std::shared_ptr<ICommandQueue> pCommandQueue)
		:
		dims_{ dims },
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
		// create rtv descriptor heap
		{
			const D3D12_DESCRIPTOR_HEAP_DESC desc = {
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
				.NumDescriptors = bufferCount_,
			};
			pDeviceInterface->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pRtvDescriptorHeap_)) >> chk;
		}
		// cache rtv descriptor size
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
		// depth buffer 
		{
			const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
			const CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_D32_FLOAT,
				(UINT)dims.width, (UINT)dims.height,
				1, 0, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
			const D3D12_CLEAR_VALUE clearValue = {
				.Format = DXGI_FORMAT_D32_FLOAT,
				.DepthStencil = { 1.0f, 0 },
			};
			pDeviceInterface->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&clearValue,
				IID_PPV_ARGS(&pDepthBuffer_)) >> chk;
		}
		// dsv descriptor heap 
		{
			const D3D12_DESCRIPTOR_HEAP_DESC desc = {
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
				.NumDescriptors = 1,
			};
			pDeviceInterface->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pDsvDescriptorHeap_)) >> chk;
		}
		// dsv and handle 
		{
			const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle{
				pDsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart() };
			pDeviceInterface->CreateDepthStencilView(pDepthBuffer_.Get(), nullptr, dsvHandle);
		}
		// scissor rect
		scissorRect_ = CD3DX12_RECT{ 0, 0, LONG_MAX, LONG_MAX };
		// viewport
		viewport_ = CD3DX12_VIEWPORT{ 0.0f, 0.0f, float(dims_.width), float(dims_.height) };
	}

	RenderPane::~RenderPane()
	{
		// wait for queue to become completely empty
		try { FlushQueues(); } catch (...) {}
	}

	void RenderPane::BeginFrame()
	{
		// set index of swap chain buffer for this frame
		curBackBufferIndex_ = pSwapChain_->GetCurrentBackBufferIndex();
		// wait for this back buffer to become free
		pCommandQueue_->WaitForFenceValue(bufferFenceValues_[curBackBufferIndex_]);
		// acquire command list
		auto commandListPair = pCommandQueue_->GetCommandListPair();
		// transition buffer resource to render target state 
		auto& backBuffer = backBuffers_[curBackBufferIndex_];
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandListPair.pCommandList->ResourceBarrier(1, &barrier);
		// clear back buffer
		if (clearColor_) {
			const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv{
				pRtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
				(INT)curBackBufferIndex_, rtvDescriptorSize_ };
			commandListPair.pCommandList->ClearRenderTargetView(rtv, &clearColor_->x, 0, nullptr);
		}
		// clear the depth buffer 
		const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle{
			pDsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart() };
		commandListPair.pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
		// execute begin frame commands
		pCommandQueue_->ExecuteCommandList(std::move(commandListPair));
	}

	CommandListPair RenderPane::GetCommandList()
	{
		// acquire command list
		auto commandListPair = pCommandQueue_->GetCommandListPair();
		// configure RS 
		commandListPair.pCommandList->RSSetViewports(1, &viewport_);
		commandListPair.pCommandList->RSSetScissorRects(1, &scissorRect_);
		// bind render target and depth buffer
		const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle{
			pDsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart() };
		const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv{
			pRtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
			(INT)curBackBufferIndex_, rtvDescriptorSize_ };
		commandListPair.pCommandList->OMSetRenderTargets(1, &rtv, TRUE, &dsvHandle);

		return commandListPair;
	}

	void RenderPane::SubmitCommandList(CommandListPair commands)
	{
		pCommandQueue_->ExecuteCommandList(std::move(commands));
	}

	uint64_t RenderPane::GetFrameFenceValue() const
	{
		return pCommandQueue_->GetFrameFenceValue();
	}

	uint64_t RenderPane::GetSignalledFenceValue() const
	{
		return pCommandQueue_->GetSignalledFenceValue();
	}

	void RenderPane::EndFrame()
	{
		auto& backBuffer = backBuffers_[curBackBufferIndex_];

		// get a command list for end frame commands
		auto commandListPair = pCommandQueue_->GetCommandListPair();
		// prepare buffer for presentation by transitioning to present state
		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				backBuffer.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			commandListPair.pCommandList->ResourceBarrier(1, &barrier);
		}
		// submit command list 
		pCommandQueue_->ExecuteCommandList(std::move(commandListPair));
		// present frame 
		pSwapChain_->Present(1, 0) >> chk;
		// insert a fence so we know when the buffer is free
		bufferFenceValues_[curBackBufferIndex_] = pCommandQueue_->SignalFrameFence();
	}

	void RenderPane::FlushQueues() const
	{
		pCommandQueue_->Flush();
	}
}
