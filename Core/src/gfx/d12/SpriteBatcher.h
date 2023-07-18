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
			if (!resourceEntryQueue_.empty() &&
				resourceEntryQueue_.front().frameFenceValue <= frameFenceValue) {
				resource = std::move(resourceEntryQueue_.front().pResource);
				resourceEntryQueue_.pop_front();
			}
			return resource;
		}
		void PutResource(T resource, uint64_t frameFenceValue)
		{
			resourceEntryQueue_.push_back(ResourceEntry_{ frameFenceValue, std::move(resource) });
		}
	private:
		struct ResourceEntry_
		{
			uint64_t frameFenceValue;
			T pResource;
		};
		std::deque<ResourceEntry_> resourceEntryQueue_;
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
		struct FrameResource_
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer_;
			D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
			Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBuffer_;
			D3D12_INDEX_BUFFER_VIEW indexBufferView_;
		};
		// functions
		FrameResource_ GetFrameResource_(uint64_t frameFenceValue);
		// connection to other gfx components
		std::shared_ptr<IDevice> pDevice_;
		// vertex stuff
		static constexpr UINT maxVertices_ = 4 * 1000;
		static constexpr UINT maxIndices_ = 6 * 1000;
		UINT nVertices_ = 0;
		UINT nIndices_ = 0;
		Vertex_* pVertexUpload_ = nullptr;
		unsigned short* pIndexUpload_ = nullptr;
		std::optional<FrameResource_> currentFrameResource_;
		FrameResourcePool<FrameResource_> frameResourcePool_;
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