#pragma once
#include "../IRenderPane.h"
#include "CommandQueue.h"
#include "Device.h"
#include <Core/src/win/IWindow.h>
#include <array>
#include "Texture.h"
#include <DirectXMath.h>

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
		struct Vertex
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT2 tc;
		};
		// data
		spa::DimensionsI dims_;
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
		// spritey
		Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer_;
		static constexpr UINT maxVertices_ = 4 * 1000;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pSrvHeap_;
		std::shared_ptr<ITexture> pTexture_;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle_;
		// pipey
		Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState_;
		D3D12_RECT scissorRect_{};
		D3D12_VIEWPORT viewport_{};
	};
}