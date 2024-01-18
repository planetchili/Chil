#include "SpriteFrame.h"


namespace chil::gfx::d12
{
	SpriteFrame::SpriteFrame(const spa::RectF& frameInPixels, size_t atlasIndex, std::shared_ptr<SpriteCodex> pCodex)
		:
		atlasIndex_{ atlasIndex },
		pCodex_{ std::move(pCodex) }
	{
		atlasDimensions_ = pCodex_->GetAtlasDimensions(atlasIndex_);
		frameInTexcoords_ = {
			.left = frameInPixels.left / atlasDimensions_.width,
			.top = frameInPixels.top / atlasDimensions_.height,
			.right = frameInPixels.right / atlasDimensions_.width,
			.bottom = frameInPixels.bottom / atlasDimensions_.height,
		};
	}

	SpriteFrame::SpriteFrame(const spa::DimensionsI& cellGridDimensions, const spa::Vec2I& cellCoordinates, size_t atlasIndex, std::shared_ptr<SpriteCodex> pCodex)
		:
		atlasIndex_{ atlasIndex },
		pCodex_{ std::move(pCodex) }
	{
		atlasDimensions_ = pCodex_->GetAtlasDimensions(atlasIndex_);
		const auto cellWidth = atlasDimensions_.width / float(cellGridDimensions.width);
		const auto cellHeight = atlasDimensions_.height / float(cellGridDimensions.height);
		const auto frameInPixels = spa::RectF{
			.left = cellWidth * cellCoordinates.x,
			.top = cellHeight * cellCoordinates.y,
			.right = cellWidth * (cellCoordinates.x + 1),
			.bottom = cellHeight * (cellCoordinates.y + 1),
		};
		frameInTexcoords_ = {
			.left = frameInPixels.left / atlasDimensions_.width,
			.top = frameInPixels.top / atlasDimensions_.height,
			.right = frameInPixels.right / atlasDimensions_.width,
			.bottom = frameInPixels.bottom / atlasDimensions_.height,
		};
	}

	void SpriteFrame::DrawToBatch(VINTERFACE(SpriteBatcher)& batch, const spa::Vec2F& pos, float rotation, const spa::Vec2F& scale) const
	{
		// deriving dest in pixel coordinates from texcoord source frame and source atlas dimensions
		const auto destPixelDims = frameInTexcoords_.GetDimensions() * atlasDimensions_;
		batch.Draw(atlasIndex_, frameInTexcoords_, destPixelDims, pos, rotation, scale);
	}
}