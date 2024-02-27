#pragma once
#include <memory>
#include <cstdint>
#include <Core/src/spa/Dimensions.h>
#include "ITexture.h"


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
	};
}