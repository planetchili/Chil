#pragma once
#include <memory>
#include <cstdint>
#include <vector>
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
		struct Atlas
		{
			Atlas(uint32_t i) : index{ i } {}
			virtual ~Atlas() = default;
			virtual spa::DimensionsI GetDimensions() const = 0;
			uint32_t index;
		};
		// functions
		virtual ~ISpriteCodex() = default;
		virtual std::shared_ptr<Atlas> AddAtlas(std::shared_ptr<ITexture> pTexture) = 0;
		virtual std::vector<std::shared_ptr<Atlas>> GetAtlases() const = 0;
	};
}