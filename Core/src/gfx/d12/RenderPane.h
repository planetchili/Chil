#pragma once
#include "../IRenderPane.h"
#include "Device.h"
#include <Core/src/win/IWindow.h>
#include <array>

namespace chil::gfx::d12
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
		~RenderPane();
		void BeginFrame() override;
		void EndFrame() override;
		void Clear(const std::array<float, 4>& color) override;
	private:
		// data
		std::shared_ptr<IDevice> pDevice_;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> pCommandQueue_;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator_;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList_;
		uint64_t fenceValue_ = 0;
		Microsoft::WRL::ComPtr<ID3D12Fence> pFence_;
		static constexpr UINT bufferCount_ = 2;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> pSwapChain_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pRtvDescriptorHeap_;
		UINT rtvDescriptorSize_;
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffers_[bufferCount_];
		UINT curBackBufferIndex_ = 0;
	};
}