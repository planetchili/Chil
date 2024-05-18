#pragma once
#include "Device.h"
#include "Texture.h"
#include "../ISpriteCodex.h"
#include "../ISpriteFrame.h"
#include "CommandListPair.h"
#include "StaticBuffer.h"
#include <memory>
#include <vector>


namespace chil::gfx::d12
{
	class ISpriteCodex : public gfx::ISpriteCodex
	{
	public:
		virtual ID3D12DescriptorHeap* GetHeap() const = 0;
		virtual D3D12_GPU_DESCRIPTOR_HANDLE GetTableHandle() const = 0;
	};

	class SpriteFrame : public ISpriteFrame
	{
	public:
		struct alignas(16) Data
		{
			spa::Vec2F pivotPixelCoords;
			spa::Vec2F frameTexPos;
			spa::DimensionsF frameTexDims;
			spa::DimensionsT<uint16_t> destPixelDims;
			uint32_t atlasIndex;
		};
		SpriteFrame(uint16_t frameIndex, const spa::RectF& frameInPixels, size_t atlasIndex, std::shared_ptr<ISpriteCodex> pCodex);
		SpriteFrame(uint16_t frameIndex, const spa::DimensionsI& cellDimension, const spa::Vec2I& cellCoordinates, size_t atlasIndex, std::shared_ptr<ISpriteCodex> pCodex);
		virtual Data PoopData() const;
		void DrawToBatch(ISpriteBatcher& batch, const spa::Vec2F& pos, float rotation = 0.f, const spa::DimensionsF& scale = { 1.f, 1.f }) const override;
	private:
		// we want to preserve pixels from src to dst
		// we can then draw to dest using src and position ONLY (and optionally scale/rotate)
		// options: frame stores src in pixel dims => convert to texcoord during drawing
		// either way, we need the atlas dimensions along with the src rect
		// we need reference to the atlas (texture) anyways, so get dimensions from there I guess?
		// we might want the flexibility of doing scale/rotate in the vertex shader, keep it in mind when placing things
		spa::RectF frameInTexcoords_;
		spa::DimensionsF atlasDimensions_;
		spa::Vec2F pivotInPixelCoordinates_;
		uint16_t atlasIndex_;
		uint16_t frameIndex_;
		std::weak_ptr<ISpriteCodex> pCodex_;
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

	class SpriteCodex : public ISpriteCodex, public std::enable_shared_from_this<SpriteCodex>
	{
	public:
		SpriteCodex(std::shared_ptr<IDevice> pDevice, UINT maxNumAtlases = 4);
		void AddSpriteAtlas(std::shared_ptr<gfx::ITexture> pTexture) override;
		ID3D12DescriptorHeap* GetHeap() const override;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTableHandle() const override;
		spa::DimensionsI GetAtlasDimensions(size_t atlasIndex) const override;
		std::shared_ptr<ISpriteFrame> AddFrame(const spa::RectF& frameInPixels, size_t atlasIndex) override;
		std::shared_ptr<ISpriteFrame> AddFrame(const spa::DimensionsI& cellDimension, const spa::Vec2I& cellCoordinates, size_t atlasIndex) override;
		virtual bool NeedsFinalization() const;
		virtual void Finalize(CommandListPair& clp, uint64_t frameFenceValue);
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
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap_;
		std::unique_ptr<StaticConstantBuffer> pConstantBuffer_;
		std::vector<std::unique_ptr<SpriteAtlas_>> spriteAtlases_;
		std::vector<std::shared_ptr<SpriteFrame>> spriteFrames_;
	};
}