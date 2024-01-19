#pragma once
#include <memory>
#include <Core/src/spa/Dimensions.h>
#include "ITexture.h"


namespace chil::gfx
{
	class ISpriteCodex
	{
	public:
		virtual ~ISpriteCodex() = default;
		virtual void AddSpriteAtlas(std::shared_ptr<ITexture> pTexture) = 0;
		virtual spa::DimensionsI GetAtlasDimensions(size_t atlasIndex) const = 0;
	};
}