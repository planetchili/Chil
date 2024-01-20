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
		maxIndices_{ 6 * maxSpriteCount },
		maxVertices_{ 4 * maxSpriteCount },
		cameraTransform_{ DirectX::XMMatrixIdentity() }
	{		
		auto pDeviceInterface = pDevice_->GetD3D12DeviceInterface();
		// root signature
		{
			// define root signature a table of sprite atlas textures
			// in future to reduce root signature binding this should just be merged into a global root descriptor
			// might want to use a bounded range, in which case the root signature will need to be updated when atlases are added
			CD3DX12_ROOT_PARAMETER rootParameters[2]{};
			const CD3DX12_DESCRIPTOR_RANGE descRange{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0 };
			// sprite codex
			rootParameters[0].InitAsDescriptorTable(1, &descRange);
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
				{ "POSITION",		0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",		0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TRANSLATION",	0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "SCALE",			0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "ROTATION",		0, DXGI_FORMAT_R32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "ATLASINDEX",		0, DXGI_FORMAT_R16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

			// create index buffer resource
			{
				const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * maxIndices_);
				pDeviceInterface->CreateCommittedResource(
					&heapProps,
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_COPY_DEST,
					nullptr, IID_PPV_ARGS(&pIndexBuffer_)
				) >> chk;
			}
			// index buffer view
			indexBufferView_ = {
				.BufferLocation = pIndexBuffer_->GetGPUVirtualAddress(),
				.SizeInBytes = (UINT)sizeof(UINT) * maxIndices_,
				.Format = DXGI_FORMAT_R32_UINT,
			};
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
		// vertex buffer
		currentFrameResource_->pVertexBuffer->Map(0, &mapReadRangeNone,
			reinterpret_cast<void**>(&pVertexUpload_)) >> chk;
		// write indices reset
		nVertices_ = 0;
		nIndices_ = 0;
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

	void SpriteBatcher::Draw(size_t atlasIndex,
		const spa::RectF& srcInTexcoords,
		const spa::DimensionsF& destPixelDims,
		const spa::Vec2F& pos,
		const float rot,
		const spa::Vec2F& scale)
	{
		using namespace DirectX;

		chilass(nVertices_ + 4 <= maxVertices_);
		chilass(nIndices_ + 6 <= maxIndices_);
		
		// atlas index 32-bit
		const auto atlasIndex32 = (UINT)atlasIndex;

		// starting dest vertice vectors
		const DirectX::XMFLOAT3 posArray[4]{
			{ 0.f, 0.f, 0.f },
			{ destPixelDims.width, 0.f, 0.f },
			{ 0.f, -destPixelDims.height, 0.f },
			{ destPixelDims.width, -destPixelDims.height, 0.f },
		};

		// use a system memory cache for building vertices before writing to UWCM
		Vertex_ vertexCache[4];
		
		// update index count
		nIndices_ += 6;

		// write vertex source coordinates
		vertexCache[0].tc = { srcInTexcoords.left, srcInTexcoords.top };
		vertexCache[1].tc = { srcInTexcoords.right, srcInTexcoords.top };
		vertexCache[2].tc = { srcInTexcoords.left, srcInTexcoords.bottom };
		vertexCache[3].tc = { srcInTexcoords.right, srcInTexcoords.bottom };

		// write vertex destination and atlas index
		for (int i = 0; i < 4; i++) {
			auto& vtx = vertexCache[i];
			vtx.position = posArray[i];
			vtx.rotation = rot;
			vtx.scale = { scale.x, scale.y };
			vtx.translation = { pos.x, pos.y };
			vtx.atlasIndex = atlasIndex32;
		}

		// copy from system cache to write-combining memory
		memcpy(&pVertexUpload_[nVertices_], vertexCache, sizeof(vertexCache));

		// increment vertex write index / count
		nVertices_ += 4;
	}

	void SpriteBatcher::EndBatch(gfx::IRenderPane& pane)
	{
		chilass(cmd_.pCommandAllocator);
		chilass(cmd_.pCommandList);

		// fill index buffer if not already filled
		if (!indexBufferFilled_) {
			WriteIndexBufferFillCommands_(cmd_);
		}
		else if (pIndexUploadBuffer_ && signaledFenceValue_ >= indexBufferUploadFenceValue_) {
			// remove upload buffer when upload is finished
			pIndexUploadBuffer_.Reset();
		}
		// unmap upload vertex
		{
			const auto mapWrittenRange = CD3DX12_RANGE{ 0, nVertices_ * sizeof(Vertex_) };
			currentFrameResource_->pVertexBuffer->Unmap(0, &mapWrittenRange);
			pVertexUpload_ = nullptr;
		}

		// set pipeline state 
		cmd_.pCommandList->SetPipelineState(pPipelineState_.Get());
		cmd_.pCommandList->SetGraphicsRootSignature(pRootSignature_.Get());
		// configure IA 
		cmd_.pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd_.pCommandList->IASetVertexBuffers(0, 1, &currentFrameResource_->vertexBufferView);
		cmd_.pCommandList->IASetIndexBuffer(&indexBufferView_);
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
		cmd_.pCommandList->DrawIndexedInstanced(nIndices_, 1, 0, 0, 0);

		// return frame resource to pool
		frameResourcePool_.PutResource(std::move(*currentFrameResource_), frameFenceValue_);
		currentFrameResource_.reset();

		// submit command list to the queue of the passed-in pane
		dynamic_cast<d12::IRenderPane&>(pane).SubmitCommandList(std::move(cmd_));
	}

	void SpriteBatcher::WriteIndexBufferFillCommands_(CommandListPair& cmd)
	{
		// create array of index data
		std::vector<UINT> indexData(maxIndices_);
		{
			UINT baseVertexIndex_ = 0;
			for (size_t i = 0; i < maxIndices_; i += 6) {
				indexData[i + 0] = baseVertexIndex_ + 0;
				indexData[i + 1] = baseVertexIndex_ + 1;
				indexData[i + 2] = baseVertexIndex_ + 2;
				indexData[i + 3] = baseVertexIndex_ + 1;
				indexData[i + 4] = baseVertexIndex_ + 3;
				indexData[i + 5] = baseVertexIndex_ + 2;
				baseVertexIndex_ += 4;
			}
		}
		// create committed resource for cpu upload of index data
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexData.size() * sizeof(UINT));
			pDevice_->GetD3D12DeviceInterface()->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&pIndexUploadBuffer_)
			) >> chk;
		}
		// copy array of index data to upload buffer  
		{
			UINT* mappedIndexData = nullptr;
			pIndexUploadBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndexData)) >> chk;
			rn::copy(indexData, mappedIndexData);
			pIndexUploadBuffer_->Unmap(0, nullptr);
		}
		// copy upload buffer to index buffer  
		cmd.pCommandList->CopyResource(pIndexBuffer_.Get(), pIndexUploadBuffer_.Get());
		// transition index buffer to index buffer state 
		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				pIndexBuffer_.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			cmd.pCommandList->ResourceBarrier(1, &barrier);
		}

		// set index buffer filled flag
		indexBufferFilled_ = true;
		// set fence value for upload complete
		indexBufferUploadFenceValue_ = frameFenceValue_;
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
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex_) * maxVertices_);
			pDeviceInterface->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&fr.pVertexBuffer)
			) >> chk;
		}
		// vertex buffer view
		fr.vertexBufferView = {
			.BufferLocation = fr.pVertexBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = (UINT)sizeof(Vertex_) * maxVertices_,
			.StrideInBytes = sizeof(Vertex_),
		};

		return fr;
	}
}