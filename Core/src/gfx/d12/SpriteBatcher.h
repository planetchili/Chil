#pragma once
#include "../IRenderPane.h"
#include "CommandQueue.h"
#include "Device.h"
#include <Core/src/win/IWindow.h>
#include <array>
#include "Texture.h"
#include <DirectXMath.h>
#include <Core/src/spa/Rect.h>
#include "../FrameResourcePool.h"
#include "SpriteCodex.h"

#include "../../Virtual.h"


namespace chil::gfx::d12
{
	class ISpriteBatcher
	{
	public:
		virtual ~ISpriteBatcher() = default;
		virtual void StartBatch(CommandListPair cmd, uint64_t frameFenceValue, uint64_t signaledFenceValue) = 0;
		virtual void SetCamera(const spa::Vec2F& pos, float rot, float scale) = 0;
		virtual void Draw(size_t atlasIndex,
			const spa::RectF& srcInTexcoords,
			const spa::DimensionsF& destPixelDims,
			const spa::Vec2F& pos,
			const float rot = 0.f,
			const spa::Vec2F& scale = { 1.f, 1.f }) = 0;
		virtual CommandListPair EndBatch() = 0;
	};

	class SpriteBatcher VSELECT(: public ISpriteBatcher)
	{
	public:
		SpriteBatcher(const spa::DimensionsI& targetDimensions, std::shared_ptr<IDevice> pDevice,
			std::shared_ptr<SpriteCodex> pSpriteCodex_, UINT maxSpriteCount = 4000);
		~SpriteBatcher();
		void StartBatch(CommandListPair cmd, uint64_t frameFenceValue, uint64_t signaledFenceValue) VOVERRIDE;
		void SetCamera(const spa::Vec2F& pos, float rot, float scale) VOVERRIDE;
		void Draw(size_t atlasIndex,
			const spa::RectF& srcInTexcoords,
			const spa::DimensionsF& destPixelDims,
			const spa::Vec2F& pos,
			const float rot = 0.f,
			const spa::Vec2F& scale = { 1.f, 1.f }) VOVERRIDE;
		CommandListPair EndBatch() VOVERRIDE;
	private:		
		// types
		struct Vertex_
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT2 tc;
			DirectX::XMFLOAT2 translation;
			DirectX::XMFLOAT2 scale;
			float rotation;
			USHORT atlasIndex;
		};
		struct FrameResource_
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer;
			D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
		};
		// functions
		FrameResource_ GetFrameResource_(uint64_t frameFenceValue);
		void WriteIndexBufferFillCommands_(CommandListPair& cmd);
		// connection to other gfx components
		std::shared_ptr<IDevice> pDevice_;
		// vertex stuff
		UINT maxVertices_;
		UINT maxIndices_;
		UINT nVertices_ = 0; 
		UINT nIndices_ = 0;
		Vertex_* pVertexUpload_ = nullptr;
		std::optional<FrameResource_> currentFrameResource_;
		FrameResourcePool<FrameResource_> frameResourcePool_;
		Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBuffer_;
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		bool indexBufferFilled_ = false;
		Microsoft::WRL::ComPtr<ID3D12Resource> pIndexUploadBuffer_;
		uint64_t indexBufferUploadFenceValue_ = 0;
		// sprite atlas codex
		std::shared_ptr<SpriteCodex> pSpriteCodex_;
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