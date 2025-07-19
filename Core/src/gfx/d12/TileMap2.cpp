#pragma once
#include "TileMap2.h"
#include "WrapD3DX.h"
#include "SpriteCodex.h"
#include "RenderPane.h"
#include <Core/src/log/Log.h>
#include <Core/src/utl/String.h>
#include <Core/src/utl/HrChecker.h>
#include <Core/src/crn/RangeBits.h>
#include <d3dcompiler.h>
#include <Core/src/utl/Assert.h>
#include <span>

namespace chil::gfx::d12
{
	using Microsoft::WRL::ComPtr;
	using utl::chk;
	namespace rn = std::ranges;

	class TileMapEffect : public ITileMapBatcherEffect
	{
	public:
		TileMapEffect(std::shared_ptr<IDevice> pDevice);
		void Bind(ID3D12GraphicsCommandList& cmdList) override;
	private:
		Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState_;
	};

	TileMapBatcher::TileMapBatcher(spa::DimensionsI blockDims, uint32_t tileSize, uint32_t sheetSize,
		std::shared_ptr<gfx::IDevice> pDevice, std::shared_ptr<gfx::ISpriteCodex> pSpriteCodex)
		:
		pDevice_{ std::dynamic_pointer_cast<decltype(pDevice_)::element_type>(std::move(pDevice)) },
		pSpriteCodex_{ std::dynamic_pointer_cast<decltype(pSpriteCodex_)::element_type>(std::move(pSpriteCodex)) },
		vertexBuffer_{ *pDevice_, crn::SpanTemp(std::vector<Vertex_>{
			{ { 0.f, 1.f } },
			{ { 1.f, 1.f } },
			{ { 0.f, 0.f } },
			{ { 1.f, 0.f } },
		}) },
		instanceFixedBuffer_{ *pDevice_, crn::SpanTemp(InstanceFixed_::MakeBufferData(blockDims)) },
		indexBuffer_{ *pDevice_, crn::SpanTemp(std::vector<USHORT>{
			0, 1, 2, 1, 3, 2,
		}) },
		blockDims_{ blockDims },
		tileSize_{ tileSize },
		sheetSize_{ sheetSize }
	{
		// setup layer constants (these will likely be changeable in the future)
		layerConstants_.tileSizeTc = float(sheetSize_) / float(tileSize_);
		layerConstants_.tileSizeWorld = float(tileSize_);
		// temporary construction of injected component
		pEffect_ = std::make_shared<TileMapEffect>(pDevice_);
		// Initialize the camera to a neutral default
		SetCamera({}, 0, 1.f);
	}

	void TileMapBatcher::StartBatch(gfx::IRenderPane& pane)
	{
		// command list/queue stuff
		auto& d12pane = dynamic_cast<d12::IRenderPane&>(pane);
		cmd_ = d12pane.GetCommandList();
		frameFenceValue_ = d12pane.GetFrameFenceValue();
		signaledFenceValue_ = d12pane.GetSignalledFenceValue();
		// handle transient gpu initialization of buffers
		// fill index buffer if not already filled
		if (!staticBuffersFilled_) {
			// initializing the static buffers
			indexBuffer_.WriteCopyCommands(cmd_, frameFenceValue_);
			vertexBuffer_.WriteCopyCommands(cmd_, frameFenceValue_);
			instanceFixedBuffer_.WriteCopyCommands(cmd_, frameFenceValue_);
			// set static buffer filled flag and completion fence value
			staticBuffersFilled_ = true;
			staticBufferUploadFenceValue_ = frameFenceValue_;
		}
		else if (signaledFenceValue_ >= staticBufferUploadFenceValue_) {
			// remove upload buffers when upload is finished
			indexBuffer_.CollectGarbage(signaledFenceValue_);
			vertexBuffer_.CollectGarbage(signaledFenceValue_);
			instanceFixedBuffer_.CollectGarbage(signaledFenceValue_);
		}
		// set pipeline state
		pEffect_->Bind(*cmd_.pCommandList.Get());
		// configure IA (block invariant portion)
		cmd_.pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		{
			const std::array views = { vertexBuffer_.GetView(), instanceFixedBuffer_.GetView() };
			cmd_.pCommandList->IASetVertexBuffers(0, 2, views.data());
		}
		cmd_.pCommandList->IASetIndexBuffer(&indexBuffer_.GetView());
		// bind the heap containing the texture descriptor
		{
			ID3D12DescriptorHeap* heapArray[1] = { pSpriteCodex_->GetHeap() };
			cmd_.pCommandList->SetDescriptorHeaps(1, heapArray);
		}
		// bind the descriptor table containing the texture descriptor
		cmd_.pCommandList->SetGraphicsRootDescriptorTable(0, pSpriteCodex_->GetTableHandle());
		// bind the per-layer (batch) root constants
		cmd_.pCommandList->SetGraphicsRoot32BitConstants(1, sizeof(layerConstants_) / 4, &layerConstants_, 0);
	}

	void TileMapBatcher::SetCamera(const spa::Vec2F& pos, float rot, float scale)
	{
		using namespace DirectX;

		// xform: translate
		auto transform = XMMatrixTranslation(-pos.x, -pos.y, 0.f);
		// xform: rotate
		transform = transform * XMMatrixRotationZ(-rot);
		// xform: scale
		transform = transform * XMMatrixScaling(scale, scale, 1.f);
		// xform: to ndc
		transform = transform * XMMatrixScaling(2.f / outputDims_.width, 2.f / outputDims_.height, 1.f);
		// column major for
		layerConstants_.cameraTransform = XMMatrixTranspose(transform);
	}

	void TileMapBatcher::DrawBlock(const gfx::ITileBlock& block)
	{
		using namespace DirectX;
		auto& blockD12 = dynamic_cast<const d12::TileBlock&>(block);
		// bind per-block constants
		const auto blockWorldPos = blockD12.GetWorldPosition();
		cmd_.pCommandList->SetGraphicsRoot32BitConstants(2, sizeof(spa::Vec2F) / sizeof(float), &blockWorldPos, 0);
		// bind per-block instance buffer
		{
			const std::array views = { blockD12.GetInstanceTileView() };
			cmd_.pCommandList->IASetVertexBuffers(2, 1, views.data());
		}
		// draw vertices
		cmd_.pCommandList->DrawIndexedInstanced(6, (UINT)blockDims_.GetArea(), 0, 0, 0);
	}

	void TileMapBatcher::EndBatch(gfx::IRenderPane& pane)
	{
		chilass(cmd_.pCommandAllocator);
		chilass(cmd_.pCommandList);
		// submit command list to the queue of the passed-in pane
		dynamic_cast<d12::IRenderPane&>(pane).SubmitCommandList(std::move(cmd_));
	}


	// effects

	TileMapEffect::TileMapEffect(std::shared_ptr<IDevice> pDevice)
	{
		auto pDeviceInterface = pDevice->GetD3D12DeviceInterface();
		// root signature
		{
			// define root signature a table of sprite atlas textures
			// in future to reduce root signature binding this should just be merged into a global root descriptor
			// might want to use a bounded range, in which case the root signature will need to be updated when atlases are added
			CD3DX12_ROOT_PARAMETER rootParameters[3]{};
			// sprite codex
			const CD3DX12_DESCRIPTOR_RANGE descRange{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0 };
			rootParameters[0].InitAsDescriptorTable(1, &descRange);
			// frame constants (gotta fix this hardcoded sizing)
			rootParameters[1].InitAsConstants(20, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
			// tilemap offset
			rootParameters[2].InitAsConstants(sizeof(spa::Vec2F) / sizeof(float), 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
			// Allow input layout and vertex shader and deny unnecessary access to certain pipeline stages.
			const D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
			// define static sampler
			const CD3DX12_STATIC_SAMPLER_DESC staticSampler{ 0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR };
			// define root signature with transformation matrix
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(
				(UINT)std::size(rootParameters), rootParameters,
				1, &staticSampler,
				rootSignatureFlags
			);
			// serialize root signature 
			ComPtr<ID3DBlob> signatureBlob;
			ComPtr<ID3DBlob> errorBlob;
			if (const auto hr = D3D12SerializeRootSignature(
				&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
				&signatureBlob, &errorBlob); FAILED(hr)) {
				if (errorBlob) {
					auto errorBufferPtr = static_cast<const char*>(errorBlob->GetBufferPointer());
					chilog.error(utl::ToWide(errorBufferPtr)).no_trace();
				}
				hr >> chk;
			}
			// Create the root signature. 
			pDeviceInterface->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
				signatureBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature_)) >> chk;
		}
		// pso (with shaders)
		{
			// static declaration of pso stream structure 
			struct PipelineStateStream
			{
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
				CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
				CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
				CD3DX12_PIPELINE_STATE_STREAM_VS VS;
				CD3DX12_PIPELINE_STATE_STREAM_PS PS;
				CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
				CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
			} pipelineStateStream;

			// define the Vertex input layout 
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
				{ "POSITION",		0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

				{ "GRIDPOS",		0, DXGI_FORMAT_R32G32_FLOAT,		1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },

				{ "ATLASTILE",		0, DXGI_FORMAT_R32_UINT,			2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			};

			// Load the vertex shader. 
			ComPtr<ID3DBlob> pVertexShaderBlob;
			D3DReadFileToBlob(L"TileVS.cso", &pVertexShaderBlob) >> chk;

			// Load the pixel shader. 
			ComPtr<ID3DBlob> pPixelShaderBlob;
			D3DReadFileToBlob(L"TilePS.cso", &pPixelShaderBlob) >> chk;

			// filling pso structure 
			pipelineStateStream.RootSignature = pRootSignature_.Get();
			pipelineStateStream.InputLayout = { inputLayout, (UINT)std::size(inputLayout) };
			pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(pVertexShaderBlob.Get());
			pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pPixelShaderBlob.Get());
			pipelineStateStream.RTVFormats = {
				.RTFormats{ DXGI_FORMAT_R8G8B8A8_UNORM },
				.NumRenderTargets = 1,
			};
			pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;

			// building the pipeline state object 
			const D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
				sizeof(PipelineStateStream), &pipelineStateStream
			};
			pDeviceInterface->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pPipelineState_)) >> chk;
		}
	}

	void TileMapEffect::Bind(ID3D12GraphicsCommandList& cmdList)
	{
		cmdList.SetPipelineState(pPipelineState_.Get());
		cmdList.SetGraphicsRootSignature(pRootSignature_.Get());
	}
}