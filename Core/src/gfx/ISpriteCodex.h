#pragma once
#include <memory>
#include <cstdint>
#include <Core/src/spa/Dimensions.h>
#include <Core/src/spa/Rect.h>
#include "ITexture.h"
#include "ISpriteFrame.h"


namespace chil::gfx
{
	class ISpriteCodex
	{
	public:
		// types
		struct IocParams
		{
			uint32_t maxAtlases;
		};
		// functions
		virtual ~ISpriteCodex() = default;
		virtual void AddSpriteAtlas(std::shared_ptr<ITexture> pTexture) = 0;
		virtual spa::DimensionsI GetAtlasDimensions(size_t atlasIndex) const = 0;
		virtual std::shared_ptr<ISpriteFrame> AddFrame(const spa::RectF& frameInPixels, size_t atlasIndex) = 0;
		virtual std::shared_ptr<ISpriteFrame> AddFrame(const spa::DimensionsI& cellDimension, const spa::Vec2I& cellCoordinates, size_t atlasIndex) = 0;
	};
}