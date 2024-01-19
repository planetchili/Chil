#pragma once
#include <memory>
#include "ISpriteBatcher.h"
#include "ISpriteCodex.h"

namespace chil::gfx
{
	class ISpriteFrame
	{
	public:
		virtual ~ISpriteFrame() = default;
		virtual void DrawToBatch(ISpriteBatcher& batch, const spa::Vec2F& pos, float rotation = 0.f, const spa::Vec2F& scale = { 1.f, 1.f }) const = 0;
	};

	class SpriteFrame : public ISpriteFrame
	{
	public:
		SpriteFrame(const spa::RectF& frameInPixels, size_t atlasIndex, std::shared_ptr<ISpriteCodex> pCodex);
		SpriteFrame(const spa::DimensionsI & cellDimension, const spa::Vec2I & cellCoordinates, size_t atlasIndex, std::shared_ptr<ISpriteCodex> pCodex);
		void DrawToBatch(ISpriteBatcher& batch, const spa::Vec2F & pos, float rotation = 0.f, const spa::Vec2F & scale = { 1.f, 1.f }) const override;
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
		std::shared_ptr<ISpriteCodex> pCodex_;
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
}