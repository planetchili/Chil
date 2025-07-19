#pragma once
#include "../TileMap.h"
#include "../IDevice.h"
#include "RenderPane.h"
#include "SpriteCodex.h"
#include "CommandQueue.h"
#include "StaticBuffer.h"
#include "Device.h"
#include "Texture.h"
#include <DirectXMath.h>

namespace chil::gfx::d12
{
	class TileBlock : public gfx::ITileBlock
	{
	public:
		TileBlock(std::span<const Tile> tiles, std::shared_ptr<IDevice> pDevice, std::shared_ptr<IRenderPane> pPane, spa::Vec2F worldPos)
			:
			instanceTileBuffer_{ *pDevice, tiles },
			worldPos_{ worldPos }
		{
			auto& pane = dynamic_cast<d12::IRenderPane&>(*pPane);
			auto clp = pane.GetCommandList();
			instanceTileBuffer_.WriteCopyCommands(clp, pane.GetFrameFenceValue());
			// TODO: waiting on a fence value from pane
			// or accessing queue from pane
			// better yet, central management of a queue for resource loading
			pane.SubmitCommandList(std::move(clp));
		}
		virtual const D3D12_VERTEX_BUFFER_VIEW& GetInstanceTileView() const
		{
			return instanceTileBuffer_.GetView();
		}
		virtual spa::Vec2F GetWorldPosition() const
		{
			return worldPos_;
		}
	private:
		StaticCpuBuffer<D3D12_VERTEX_BUFFER_VIEW> instanceTileBuffer_;
		spa::Vec2F worldPos_;
	};

	class ITileMapBatcherEffect
	{
	public:
		virtual ~ITileMapBatcherEffect() = default;
		virtual void Bind(ID3D12GraphicsCommandList& cmdList) = 0;
	};

	class TileMapBatcher : public gfx::ITileMapBatcher
	{
	public:
		TileMapBatcher(spa::DimensionsI blockDims, uint32_t tileSize, uint32_t sheetSize,
			std::shared_ptr<gfx::IDevice> pDevice, std::shared_ptr<gfx::ISpriteCodex> pSpriteCodex);
		void StartBatch(gfx::IRenderPane& pane) override;
		void SetCamera(const spa::Vec2F& pos, float rot, float scale) override;
		void DrawBlock(const gfx::ITileBlock& block) override;
		void EndBatch(gfx::IRenderPane& pane) override;
	private:
		// types
		struct Vertex_
		{
			DirectX::XMFLOAT2 position;
		};
		struct InstanceFixed_
		{
			float cellX;
			float cellY;
			static std::vector<InstanceFixed_> MakeBufferData(spa::DimensionsI blockDims)
			{
				std::vector<InstanceFixed_> data;
				for (float y = 0.f; y < (float)blockDims.height; y += 1.f) {
					for (float x = 0.f; x < (float)blockDims.width; x += 1.f) {
						data.emplace_back(x, y);
					}
				}
				return data;
			}
		};
		struct LayerConstants_
		{
			DirectX::XMMATRIX cameraTransform;
			float tileSizeTc;
			float tileSizeWorld;
		} layerConstants_;
		// connection to other gfx components
		std::shared_ptr<d12::IDevice> pDevice_;
		// sprite atlas codex
		std::shared_ptr<d12::SpriteCodex> pSpriteCodex_;
		// command list (moved in/out)
		CommandListPair cmd_;
		uint64_t frameFenceValue_ = 0;
		uint64_t signaledFenceValue_ = 0;
		// geometry
		spa::DimensionsI blockDims_;
		uint32_t tileSize_;
		uint32_t sheetSize_;
		StaticCpuBuffer<D3D12_INDEX_BUFFER_VIEW> indexBuffer_;
		StaticCpuBuffer<D3D12_VERTEX_BUFFER_VIEW> vertexBuffer_;
		StaticCpuBuffer<D3D12_VERTEX_BUFFER_VIEW> instanceFixedBuffer_;
		bool staticBuffersFilled_ = false;
		uint64_t staticBufferUploadFenceValue_ = 0;
		// pipey
		spa::DimensionsF outputDims_;
		std::shared_ptr<ITileMapBatcherEffect> pEffect_;
	};
}