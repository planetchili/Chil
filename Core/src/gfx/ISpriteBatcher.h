#pragma once
#include <memory>
#include "IRenderPane.h"
#include "ISpriteCodex.h"
#include <Core/src/spa/Vec2.h>
#include <Core/src/spa/Rect.h>

namespace chil::gfx
{
	class ISpriteBatcher
	{
	public:
		// types
		struct IocParams
		{
			spa::DimensionsI targetDimensions;
			std::shared_ptr<gfx::ISpriteCodex> pSpriteCodex;
			UINT maxSpriteCount = 4000;
		};
		// functions
		virtual ~ISpriteBatcher() = default;
		virtual void StartBatch(IRenderPane& pane) = 0;
		virtual void SetCamera(const spa::Vec2F& pos, float rot, float scale) = 0;
		virtual void Draw(size_t atlasIndex,
			const spa::RectF& srcInTexcoords,
			const spa::DimensionsF& destPixelDims,
			const spa::Vec2F& pos,
			const float rot = 0.f,
			const spa::DimensionsF& scale = { 1.f, 1.f }) = 0;
		virtual void EndBatch(IRenderPane& pane) = 0;
	};
}
