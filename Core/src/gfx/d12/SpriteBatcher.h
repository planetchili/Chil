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
		virtual void Draw(size_t atlasIndex,
			const spa::RectF& srcInTexcoords,
			const spa::DimensionsF& destPixelDims,
			const spa::Vec2F& pos,
			const float rot = 0.f,
			const spa::Vec2F& scale = { 1.f, 1.f }) = 0;
		virtual CommandListPair EndBatch() = 0;
	};

	class SpriteCodex
	{
	public:
		SpriteCodex(std::shared_ptr<IDevice> pDevice, UINT maxNumAtlases = 4);
		void AddSpriteAtlas(std::shared_ptr<ITexture> pTexture);
		ID3D12DescriptorHeap* GetHeap() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTableHandle() const;
		spa::DimensionsI GetAtlasDimensions(size_t atlasIndex) const;
	private:
		// types
		struct SpriteAtlas_
		{
			D3D12_CPU_DESCRIPTOR_HANDLE srvHandle_;
			std::shared_ptr<ITexture> pTexture_;
		};
		// data
		std::shared_ptr<IDevice> pDevice_;
		UINT descriptorSize_;
		UINT maxNumAtlases_;
		UINT curNumAtlases_ = 0;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pSrvHeap_;
		std::vector<std::unique_ptr<SpriteAtlas_>> spriteAtlases_;
	};

	class SpriteFrame
	{
	public:
		SpriteFrame(const spa::RectF& frameInPixels, size_t atlasIndex, std::shared_ptr<SpriteCodex> pCodex);
		SpriteFrame(const spa::DimensionsI& cellDimension, const spa::Vec2I& cellCoordinates, size_t atlasIndex, std::shared_ptr<SpriteCodex> pCodex);
		void DrawToBatch(ISpriteBatcher& batch, const spa::Vec2F& pos, float rotation = 0.f, const spa::Vec2F& scale = { 1.f, 1.f }) const;
	private:
		// we want to preserve pixels from src to dst
		// we can then draw to dest using src and position ONLY (and optionally scale/rotate)
		// options: frame stores src in pixel dims => convert to texcoord during drawing
		// either way, we need the atlas dimensions along with the src rect
		// we need reference to the atlas (texture) anyways, so get dimensions from there I guess?
		// we might want the flexibility of doing scale/rotate in the vertex shader, keep it in mind when placing things
		spa::RectF frameInTexcoords_;
		spa::DimensionsF atlasDimensions_;
		size_t atlasIndex_;
		std::shared_ptr<SpriteCodex> pCodex_;
		// we need a handle that connects to a SRV in a heap of the batch
		// you can have different sets of textures in multiple batches
		// how can you allow the same sprite frame to work with different batches?
		//   could use an unordered map: but maybe a little slow
		//   could use different frames per batch: annoying to try and render one scene into multiple batches
		//   could just have same textures bound to each batch
		//        pull srvs out of batches into atlas
		//		  closed ecosystem of batches, sprite frames, and 1 atlas, but you could have multiple ecosystems
		//		  might be good (at least in debug) to validate that frames are being used with valid batches
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
		SpriteBatcher(const spa::DimensionsI& targetDimensions, std::shared_ptr<IDevice> pDevice,
			std::shared_ptr<SpriteCodex> pSpriteCodex_, UINT maxSpriteCount = 4000);
		~SpriteBatcher();
		void StartBatch(CommandListPair cmd, uint64_t frameFenceValue, uint64_t signaledFenceValue) override;
		void SetCamera(const spa::Vec2F& pos, float rot, float scale) override;
		void Draw(size_t atlasIndex,
			const spa::RectF& srcInTexcoords,
			const spa::DimensionsF& destPixelDims,
			const spa::Vec2F& pos,
			const float rot = 0.f,
			const spa::Vec2F& scale = { 1.f, 1.f }) override;
		CommandListPair EndBatch() override;
	private:		
		// types
		struct Vertex_
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT2 tc;
			USHORT atlasIndex;
		};
		struct FrameResource_
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer;
			D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
			Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBuffer;
			D3D12_INDEX_BUFFER_VIEW indexBufferView{};
		};
		// functions
		FrameResource_ GetFrameResource_(uint64_t frameFenceValue);
		// connection to other gfx components
		std::shared_ptr<IDevice> pDevice_;
		// vertex stuff
		UINT maxVertices_;
		UINT maxIndices_;
		UINT nVertices_ = 0;
		UINT nIndices_ = 0;
		Vertex_* pVertexUpload_ = nullptr;
		unsigned short* pIndexUpload_ = nullptr;
		std::optional<FrameResource_> currentFrameResource_;
		FrameResourcePool<FrameResource_> frameResourcePool_;
		// sprite atlas codex
		std::shared_ptr<SpriteCodex> pSpriteCodex_;
		// command list (moved in/out)
		CommandListPair cmd_;
		uint64_t frameFenceValue_ = 0;
		// pipey
		Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState_;
		spa::DimensionsF outputDims_;
	};
}