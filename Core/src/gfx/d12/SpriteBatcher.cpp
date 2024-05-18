#pragma once
#include "SpriteBatcher.h"
#include "WrapD3DX.h"
#include "SpriteCodex.h"
#include "RenderPane.h"
#include <Core/src/log/Log.h>
#include <Core/src/utl/String.h>
#include <Core/src/utl/HrChecker.h>
#include <d3dcompiler.h>
#include <Core/src/utl/Assert.h>

namespace chil::gfx::d12
{
	using Microsoft::WRL::ComPtr;
	using utl::chk;
	namespace rn = std::ranges;

	SpriteBatcher::SpriteBatcher(const spa::DimensionsI& targetDimensions, std::shared_ptr<gfx::IDevice> pDevice,
		std::shared_ptr<gfx::ISpriteCodex> pSpriteCodex, UINT maxSpriteCount)
		:
		pDevice_{ std::dynamic_pointer_cast<decltype(pDevice_)::element_type>(std::move(pDevice)) },
		outputDims_{ (spa::DimensionsF)targetDimensions },
		pSpriteCodex_{ std::dynamic_pointer_cast<decltype(pSpriteCodex_)::element_type>(std::move(pSpriteCodex)) },
		maxInstances_{ maxSpriteCount },
		cameraTransform_{ DirectX::XMMatrixIdentity() },
		vertexBuffer_{ *pDevice_, std::vector<Vertex_>{
			{ { -0.5f,  0.5f  } },
			{ {  0.5f,  0.5f  } },
			{ { -0.5f, -0.5f  } },
			{ {  0.5f, -0.5f  } },
		} },
		indexBuffer_{ *pDevice_, std::vector<USHORT>{
			0, 1, 2, 1, 3, 2,
		} }
	{		
		auto pDeviceInterface = pDevice_->GetD3D12DeviceInterface();
		// root signature
		{
			// define root signature a table of sprite atlas textures
			// in future to reduce root signature binding this should just be merged into a global root descriptor
			// might want to use a bounded range, in which case the root signature will need to be updated when atlases are added
			CD3DX12_ROOT_PARAMETER rootParameters[2]{};
			// sprite codex
			const CD3DX12_DESCRIPTOR_RANGE descRanges[2] = { 
				CD3DX12_DESCRIPTOR_RANGE{ D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1 },
				CD3DX12_DESCRIPTOR_RANGE{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0 },
			};
			rootParameters[0].InitAsDescriptorTable(2, descRanges);
			// camera transform
			rootParameters[1].InitAsConstants(sizeof(DirectX::XMMATRIX) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
			// Allow input layout and vertex shader and deny unnecessary access to certain pipeline stages.
			const D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
			// define static sampler
			const CD3DX12_STATIC_SAMPLER_DESC staticSampler{ 0, D3D12_FILTER_MIN_MAG_MIP_POINT };
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

				{ "TRANSLATION",	0, DXGI_FORMAT_R32G32_FLOAT,		1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "ROTATION",		0, DXGI_FORMAT_R32_FLOAT,			1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "SCALE",			0, DXGI_FORMAT_R32G32_FLOAT,		1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "FRAMEINDEX",		0, DXGI_FORMAT_R32_UINT,			1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			};

			// Load the vertex shader. 
			ComPtr<ID3DBlob> pVertexShaderBlob;
			D3DReadFileToBlob(L"VertexShader.cso", &pVertexShaderBlob) >> chk;

			// Load the pixel shader. 
			ComPtr<ID3DBlob> pPixelShaderBlob;
			D3DReadFileToBlob(L"PixelShader.cso", &pPixelShaderBlob) >> chk;

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

	void SpriteBatcher::StartBatch(gfx::IRenderPane& pane)
	{
		// command list/queue stuff
		auto& d12pane = dynamic_cast<d12::IRenderPane&>(pane);
		cmd_ = d12pane.GetCommandList();
		frameFenceValue_ = d12pane.GetFrameFenceValue();
		signaledFenceValue_ = d12pane.GetSignalledFenceValue();
		// frame resource stuff
		currentFrameResource_ = GetFrameResource_(signaledFenceValue_);
		const auto mapReadRangeNone = CD3DX12_RANGE{ 0, 0 };
		// instance buffer
		currentFrameResource_->pBuffer->Map(0, &mapReadRangeNone,
			reinterpret_cast<void**>(&pInstanceUpload_)) >> chk;
		// write index reset
		nInstances_ = 0;
	}

	void SpriteBatcher::SetCamera(const spa::Vec2F& pos, float rot, float scale)
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
		cameraTransform_ = XMMatrixTranspose(transform);
	}

	void SpriteBatcher::Draw(size_t frameIndex,
		const spa::Vec2F& pos,
		const float rot,
		const spa::DimensionsF& scale)
	{
		using namespace DirectX;

		chilass(nInstances_ <= maxInstances_);

		// use a system memory cache for building instance struct before writing to UWCM
		Instance_ instanceCache{
			.translation = pos,
			.rotation = rot,
			.scale = scale,
			.frameIndex = (uint32_t)frameIndex,
		};

		// copy from system cache to write-combining memory
		memcpy(&pInstanceUpload_[nInstances_], &instanceCache, sizeof(instanceCache));

		// increment instance write index / count
		nInstances_++;
	}

	void SpriteBatcher::EndBatch(gfx::IRenderPane& pane)
	{
		chilass(cmd_.pCommandAllocator);
		chilass(cmd_.pCommandList);

		// fill index/vertex buffers if not already filled
		if (!staticBuffersFilled_) {
			// initializing the static buffers
			indexBuffer_.WriteCopyCommands(cmd_, frameFenceValue_);
			vertexBuffer_.WriteCopyCommands(cmd_, frameFenceValue_);
			// set static buffer filled flag and completion fence value
			staticBuffersFilled_ = true;
			staticBufferUploadFenceValue_ = frameFenceValue_;
		}
		else if (signaledFenceValue_ >= staticBufferUploadFenceValue_) {
			// remove upload buffers when upload is finished
			indexBuffer_.CollectGarbage(signaledFenceValue_);
			vertexBuffer_.CollectGarbage(signaledFenceValue_);
		}
		// write sprite codex buffers if necessary
		if (pSpriteCodex_->NeedsFinalization()) {
			pSpriteCodex_->Finalize(cmd_, frameFenceValue_);
		}
		// unmap upload instance buffer
		{
			const auto mapWrittenRange = CD3DX12_RANGE{ 0, nInstances_ * sizeof(Instance_) };
			currentFrameResource_->pBuffer->Unmap(0, &mapWrittenRange);
			pInstanceUpload_ = nullptr;
		}

		// set pipeline state 
		cmd_.pCommandList->SetPipelineState(pPipelineState_.Get());
		cmd_.pCommandList->SetGraphicsRootSignature(pRootSignature_.Get());
		// configure IA 
		cmd_.pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		{
			const std::array views = { vertexBuffer_.GetView(), currentFrameResource_->bufferView};
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
		// bind the camera transform matrix
		cmd_.pCommandList->SetGraphicsRoot32BitConstants(1, sizeof(cameraTransform_) / 4, &cameraTransform_, 0);
		// draw vertices
		cmd_.pCommandList->DrawIndexedInstanced(6, nInstances_, 0, 0, 0);

		// return frame resource to pool
		frameResourcePool_.PutResource(std::move(*currentFrameResource_), frameFenceValue_);
		currentFrameResource_.reset();

		// submit command list to the queue of the passed-in pane
		dynamic_cast<d12::IRenderPane&>(pane).SubmitCommandList(std::move(cmd_));
	}

	// SpriteBatcher::FrameResource_
	// -----------------------------

	SpriteBatcher::FrameResource_ SpriteBatcher::GetFrameResource_(uint64_t frameFenceValue)
	{
		// get an existing buffer available from the pool
		if (auto fr = frameResourcePool_.GetResource(frameFenceValue)) {
			return std::move(*fr);
		}
		// create a new one if none available
		auto pDeviceInterface = pDevice_->GetD3D12DeviceInterface();
		FrameResource_ fr;
		// vertex buffer
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Instance_) * maxInstances_);
			pDeviceInterface->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&fr.pBuffer)
			) >> chk;
		}
		// vertex buffer view
		fr.bufferView = {
			.BufferLocation = fr.pBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = (UINT)sizeof(Instance_) * maxInstances_,
			.StrideInBytes = sizeof(Instance_),
		};

		return fr;
	}
}