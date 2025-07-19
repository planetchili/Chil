#pragma once
#include <cstdint>
#include <span>
#include <Core/src/spa/Vec2.h>
#include <Core/src/spa/Dimensions.h>
#include "IRenderPane.h"
#include "IDevice.h"
#include "ISpriteCodex.h"

namespace chil::gfx
{
	struct Tile
	{
		uint16_t atlasIdx;
		uint16_t tileIdx;
	};

	class ITileBlock
	{
	public:
		virtual ~ITileBlock() = default;
	private:
	};

	class ITileMapBatcher
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
		virtual ~ITileMapBatcher() = default;
		virtual void StartBatch(IRenderPane& pane) = 0;
		virtual void SetCamera(const spa::Vec2F& pos, float rot, float scale) = 0;
		virtual void DrawBlock(const ITileBlock& block) = 0;
		virtual void EndBatch(IRenderPane& pane) = 0;
	};

	// stores the grid of blocks
	// uses camera info to determine the subrect of blocks to render
	// generates blocks given a 2d array of tiles + atlas set
	// references / shares an atlas
	class StaticTileMap
	{
	public:
		StaticTileMap(std::span<const Tile> tiles, spa::DimensionsI mapDims, spa::DimensionsI tileDims,
			spa::Vec2F topLeft, spa::DimensionsI blockDims, std::shared_ptr<ISpriteCodex> pCodex,
			std::shared_ptr<IDevice> pDevice, std::shared_ptr<IRenderPane> pPane);
		void DrawToBatch(ITileMapBatcher& batcher, IRenderPane& pane) const
		{
			batcher.StartBatch(pane);
			// TODO: grid scan to only process blocks within the viewport
			for (auto& p : blockPtrs_) {
				batcher.DrawBlock(*p);
			}
			batcher.EndBatch(pane);
		}
	private:
		spa::DimensionsI mapDims_;
		int tileSize_;
		std::vector<std::unique_ptr<ITileBlock>> blockPtrs_;
		std::shared_ptr<ISpriteCodex> pCodex_;
	};
}