#pragma once
#include "../IRenderPane.h"
#include "CommandQueue.h"
#include "Device.h"
#include <Core/src/win/IWindow.h>
#include <array>
#include "Texture.h"
#include <DirectXMath.h>
#include <Core/src/spa/Rect.h>

namespace chil::gfx::d12
{
	class ISpriteBatcher
	{
	public:
		virtual ~ISpriteBatcher() = default;
		virtual void StartBatch(CommandListPair cmd, uint64_t frameFenceValue, uint64_t signaledFenceValue) = 0;
		virtual void SetCamera(const spa::Vec2F& pos, float rot, float scale) = 0;
		virtual void Draw(const spa::RectF& src, const spa::RectF& dest) = 0;
		virtual CommandListPair EndBatch() = 0;
		virtual void AddTexture(std::shared_ptr<ITexture> pTexture) = 0;
	};

	template<class T>
	class FrameResourcePool
	{
	public:
		std::optional<T> GetResource(uint64_t frameFenceValue)
		{
			std::optional<T> resource;
			if (!commandAllocatorQueue_.empty() &&
				commandAllocatorQueue_.front().frameFenceValue <= frameFenceValue) {
				resource = std::move(commandAllocatorQueue_.front().pResource);
				commandAllocatorQueue_.pop_front();
			}
			return resource;
		}
		void PutResource(T resource, uint64_t frameFenceValue)
		{
			commandAllocatorQueue_.push_back(ResourceEntry_{ frameFenceValue, std::move(resource) });
		}
	private:
		struct ResourceEntry_
		{
			uint64_t frameFenceValue;
			T pResource;
		};
		std::deque<ResourceEntry_> commandAllocatorQueue_;
	};

	class SpriteBatcher : public ISpriteBatcher
	{
	public:
		SpriteBatcher(std::shared_ptr<IDevice> pDevice);
		~SpriteBatcher();
		void StartBatch(CommandListPair cmd, uint64_t frameFenceValue, uint64_t signaledFenceValue) override;
		void SetCamera(const spa::Vec2F& pos, float rot, float scale) override;
		void Draw(const spa::RectF& src, const spa::RectF& dest) override;
		CommandListPair EndBatch() override;
		void AddTexture(std::shared_ptr<ITexture> pTexture) override;
	private:		
		// types
		struct Vertex_
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT2 tc;
		};
		struct VertexBufferResource_
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer_;
			D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
		};
		// functions
		VertexBufferResource_ GetVertexBuffer_(uint64_t frameFenceValue);
		// connection to other gfx components
		std::shared_ptr<IDevice> pDevice_;
		// vertex stuff
		static constexpr UINT maxVertices_ = 4 * 1000;
		UINT nVertices_ = 0;
		Vertex_* pVertexUpload_ = nullptr;
		std::optional<VertexBufferResource_> currentVertexBuffer_;
		FrameResourcePool<VertexBufferResource_> vertexBufferPool_;
		// texture stuff
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pSrvHeap_;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle_;
		std::shared_ptr<ITexture> pTexture_;
		// command list (moved in/out)
		CommandListPair cmd_;
		uint64_t frameFenceValue_ = 0;
		// pipey
		Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState_;
	};
}