#pragma once
#include <memory>
#include <Core/src/spa/Vec2.h>
#include <Core/src/spa/Dimensions.h>

namespace chil::gfx
{
	class ISpriteFrame
	{
	public:
		virtual ~ISpriteFrame() = default;
		virtual void DrawToBatch(class ISpriteBatcher& batch, const spa::Vec2F& pos, float rotation = 0.f, float zOrder = 0.f, const spa::DimensionsF& scale = { 1.f, 1.f }) const = 0;
	};
}