#pragma once
#include "../IRenderPane.h"
#include "CommandQueue.h"
#include "Device.h"
#include "Texture.h"
#include <DirectXMath.h>
#include <Core/src/spa/Rect.h>
#include "SpriteCodex.h"
#include "../FrameResourcePool.h"
#include "../ISpriteBatcher.h"
#include <array>
#include "StaticBuffer.h"


namespace chil::gfx::d12
{
	class ISpriteBatcher : public gfx::ISpriteBatcher {};

	class SpriteBatcher : public ISpriteBatcher
	{
	public:
		SpriteBatcher(const spa::DimensionsI& targetDimensions, std::shared_ptr<gfx::IDevice> pDevice,
			std::shared_ptr<gfx::ISpriteCodex> pSpriteCodex_, UINT maxSpriteCount = 4000);
		void StartBatch(gfx::IRenderPane& pane) override;
		void SetCamera(const spa::Vec2F& pos, float rot, float scale) override;
		void Draw(size_t frameIndex,
			const spa::Vec2F& pos,
			const float rot = 0.f,
			const float zOrder = 0.f,
			const spa::DimensionsF& scale = { 1.f, 1.f }) override;
		void EndBatch(gfx::IRenderPane& pane) override;
	private:		
		// types
		struct Vertex_
		{
			DirectX::XMFLOAT2 position;
		};
		struct Instance_
		{
			spa::Vec2F translation;
			float rotation;
			float zOrder;
			spa::DimensionsF scale;
			uint32_t frameIndex;
		};
		struct FrameResource_
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> pBuffer;
			D3D12_VERTEX_BUFFER_VIEW bufferView{};
		};
		// functions
		FrameResource_ GetFrameResource_(uint64_t frameFenceValue);
		// connection to other gfx components
		std::shared_ptr<d12::IDevice> pDevice_;
		// vertex stuff
		UINT maxInstances_;
		UINT nInstances_ = 0;
		Instance_* pInstanceUpload_ = nullptr;
		std::optional<FrameResource_> currentFrameResource_;
		FrameResourcePool<FrameResource_> frameResourcePool_;
		StaticCpuBuffer<D3D12_INDEX_BUFFER_VIEW> indexBuffer_;
		StaticCpuBuffer<D3D12_VERTEX_BUFFER_VIEW> vertexBuffer_;
		bool staticBuffersFilled_ = false;
		uint64_t staticBufferUploadFenceValue_ = 0;
		// sprite atlas codex
		std::shared_ptr<d12::SpriteCodex> pSpriteCodex_;
		// command list (moved in/out)
		CommandListPair cmd_;
		uint64_t frameFenceValue_ = 0;
		uint64_t signaledFenceValue_ = 0;
		// pipey
		Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState_;
		spa::DimensionsF outputDims_;
		// camera
		DirectX::XMMATRIX cameraTransform_;
	};
}