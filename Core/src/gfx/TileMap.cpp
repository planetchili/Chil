#include "TileMap.h"
#include "d12/TileMap2.h"

namespace chil::gfx
{
	StaticTileMap::StaticTileMap(std::span<const Tile> tiles, spa::DimensionsI mapDims, spa::DimensionsI tileDims,
		spa::Vec2F topLeft, spa::DimensionsI blockDims, std::shared_ptr<ISpriteCodex> pCodex,
		std::shared_ptr<IDevice> pDevice, std::shared_ptr<IRenderPane> pPane)
		:
		pCodex_{ std::move(pCodex) },
		mapDims_{ mapDims },
		tileSize_{ tileSize_ }
	{
		const auto At = [&tiles, &mapDims](int x, int y) -> const Tile& {
			return tiles[x + y * mapDims.width];
		};
		// loop through blocks
		for (int yStart = 0; yStart < mapDims.height; yStart += blockDims.height) {
			for (int xStart = 0; xStart < mapDims.width; xStart += blockDims.width) {
				std::vector<Tile> blockTiles;
				// loop through tiles in each block
				for (int y = 0; y < mapDims.height; y += blockDims.height) {
					for (int x = 0; x < mapDims.width; x += blockDims.width) {
						blockTiles.push_back(At(x, y));
					}
				}
				// TODO: actually calculate the world pos
				blockPtrs_.push_back(std::make_unique<d12::TileBlock>(blockTiles,
					std::dynamic_pointer_cast<gfx::d12::IDevice>(pDevice),
					std::dynamic_pointer_cast<gfx::d12::IRenderPane>(pPane), spa::Vec2F{ 0.f, 0.f }));
			}
		}
	}
}