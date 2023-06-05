#pragma once
#include "../IRenderPane.h"
#include "CommandQueue.h"
#include "Device.h"
#include <Core/src/win/IWindow.h>
#include <array>
#include "Texture.h"

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
		RenderPane(HWND hWnd, const spa::DimensionsI& dims, std::shared_ptr<IDevice> pDevice,
			std::shared_ptr<ICommandQueue> pCommandQueue);
		~RenderPane();
		void BeginFrame() override;
		void EndFrame() override;
		void Clear(const std::array<float, 4>& color) override;
	private:
		// data
		std::shared_ptr<IDevice> pDevice_;
		std::shared_ptr<ICommandQueue> pCommandQueue_;
		CommandListPair commandListPair_;
		static constexpr UINT bufferCount_ = 2;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> pSwapChain_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pRtvDescriptorHeap_;
		UINT rtvDescriptorSize_;
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffers_[bufferCount_];
		UINT curBackBufferIndex_ = 0;
		uint64_t bufferFenceValues_[bufferCount_]{};
		std::shared_ptr<ITexture> pTexture_;
	};
}