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
		void Draw(size_t atlasIndex,
			const spa::RectF& srcInTexcoords,
			const spa::DimensionsF& destPixelDims,
			const spa::Vec2F& pos,
			const float rot = 0.f,
			const spa::Vec2F& scale = { 1.f, 1.f }) override;
		void EndBatch(gfx::IRenderPane& pane) override;
	private:		
		// types
		struct Vertex_
		{
			DirectX::XMFLOAT2 position;
		};
		struct Instance_
		{
			DirectX::XMFLOAT2 translation;
			float rotation;
			DirectX::XMFLOAT2 scale;		
			// x: left, y: top, z: right, a: bottom
			DirectX::XMFLOAT4 texRect;
			uint16_t destDimensions[2];
			uint16_t atlasIndex;
		};
		struct FrameResource_
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> pBuffer;
			D3D12_VERTEX_BUFFER_VIEW bufferView{};
		};
		// functions
		FrameResource_ GetFrameResource_(uint64_t frameFenceValue);
		void WriteStaticBufferFillCommands_(CommandListPair& cmd);
		// connection to other gfx components
		std::shared_ptr<d12::IDevice> pDevice_;
		// vertex stuff
		UINT maxInstances_;
		UINT nInstances_ = 0;
		Instance_* pInstanceUpload_ = nullptr;
		std::optional<FrameResource_> currentFrameResource_;
		FrameResourcePool<FrameResource_> frameResourcePool_;
		Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBuffer_;
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		Microsoft::WRL::ComPtr<ID3D12Resource> pIndexUploadBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer_;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		Microsoft::WRL::ComPtr<ID3D12Resource> pVertexUploadBuffer_;
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