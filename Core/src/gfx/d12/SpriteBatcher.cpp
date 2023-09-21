#pragma once
#include "SpriteBatcher.h"
#pragma warning(push)
#pragma warning(disable : 26495)
#include "d3dx12.h"
#pragma warning(pop)
#include <Core/src/log/Log.h>
#include <Core/src/utl/String.h>
#include <Core/src/utl/HrChecker.h>
#include <d3dcompiler.h>
#include <Core/src/utl/Assert.h>

namespace chil::gfx::d12
{
	using Microsoft::WRL::ComPtr;
	using utl::chk;

	SpriteBatcher::SpriteBatcher(const spa::DimensionsI& targetDimensions, std::shared_ptr<IDevice> pDevice, std::shared_ptr<SpriteCodex> pSpriteCodex)
		:
		pDevice_{ std::move(pDevice) },
		outputDims_{ (spa::DimensionsF)targetDimensions },
		pSpriteCodex_{ std::move(pSpriteCodex) }
	{		
		auto pDeviceInterface = pDevice_->GetD3D12DeviceInterface();
		// root signature
		{
			// define root signature a table of sprite atlas textures
			// in future to reduce root signature binding this should just be merged into a global root descriptor
			// might want to use a bounded range, in which case the root signature will need to be updated when atlases are added
			CD3DX12_ROOT_PARAMETER rootParameters[1]{};
			const CD3DX12_DESCRIPTOR_RANGE descRange{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0 };
			rootParameters[0].InitAsDescriptorTable(1, &descRange);
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
				CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC Blend;
			} pipelineStateStream;

			// define the Vertex input layout 
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "ATLASINDEX", 0, DXGI_FORMAT_R16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
			pipelineStateStream.Blend = [] {
				CD3DX12_BLEND_DESC blendDesc{ D3D12_DEFAULT };
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
				return blendDesc;
			}();

			// building the pipeline state object 
			const D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
				sizeof(PipelineStateStream), &pipelineStateStream
			};
			pDeviceInterface->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pPipelineState_)) >> chk;
		}
	}

	SpriteBatcher::~SpriteBatcher() = default;

	void SpriteBatcher::StartBatch(CommandListPair cmd, uint64_t frameFenceValue, uint64_t signaledFenceValue)
	{
		// command list/queue stuff
		cmd_ = std::move(cmd);
		frameFenceValue_ = frameFenceValue;
		// frame resource stuff
		currentFrameResource_ = GetFrameResource_(signaledFenceValue);
		const auto mapReadRangeNone = CD3DX12_RANGE{ 0, 0 };
		// vertex buffer
		currentFrameResource_->pVertexBuffer->Map(0,&mapReadRangeNone,
			reinterpret_cast<void**>(&pVertexUpload_)) >> chk;
		// index buffer
		currentFrameResource_->pIndexBuffer->Map(0, &mapReadRangeNone,
			reinterpret_cast<void**>(&pIndexUpload_)) >> chk;
		// write indices reset
		nVertices_ = 0;
		nIndices_ = 0;
	}

	void SpriteBatcher::SetCamera(const spa::Vec2F& pos, float rot, float scale)
	{
		chilchk_fail;
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
		
		// atlas index 16-bit
		const auto atlasIndex16 = (USHORT)atlasIndex;

		// starting dest vertice vectors
		const XMVECTOR posSimd[4]{
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(destPixelDims.width, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, -destPixelDims.height, 0.f, 1.f),
			XMVectorSet(destPixelDims.width, -destPixelDims.height, 0.f, 1.f),
		};

		// xform: scale
		auto transform = XMMatrixScaling(scale.x, scale.y, 1.f);
		// xform: rotate
		transform = transform * XMMatrixRotationZ(rot);
		// xform: translate
		transform = transform * XMMatrixTranslation(pos.x, pos.y, 0.f);
		// TODO: xform: camera
		// xform: to ndc
		const auto halfDims = outputDims_ / 2.f;
		transform = transform * XMMatrixScaling(1.f / halfDims.width, 1.f / halfDims.height, 1.f);
		
		// write indices
		pIndexUpload_[nIndices_++] = nVertices_;
		pIndexUpload_[nIndices_++] = nVertices_ + 1;
		pIndexUpload_[nIndices_++] = nVertices_ + 2;
		pIndexUpload_[nIndices_++] = nVertices_ + 1;
		pIndexUpload_[nIndices_++] = nVertices_ + 3;
		pIndexUpload_[nIndices_++] = nVertices_ + 2;

		// write vertex source coordinates
		pVertexUpload_[nVertices_ + 0].tc = { srcInTexcoords.left, srcInTexcoords.top };
		pVertexUpload_[nVertices_ + 1].tc = { srcInTexcoords.right, srcInTexcoords.top };
		pVertexUpload_[nVertices_ + 2].tc = { srcInTexcoords.left, srcInTexcoords.bottom };
		pVertexUpload_[nVertices_ + 3].tc = { srcInTexcoords.right, srcInTexcoords.bottom };

		// write vertex destination and atlas index
		for (int i = 0; i < 4; i++) {
			auto& vtx = pVertexUpload_[nVertices_ + i];
			const auto dest = XMVector4Transform(posSimd[i], transform);
			XMStoreFloat3(&vtx.position, dest);
			vtx.atlasIndex = atlasIndex16;
		}

		// increment vertex write index / count
		nVertices_ += 4;
	}

	CommandListPair SpriteBatcher::EndBatch()
	{
		chilass(cmd_.pCommandAllocator);
		chilass(cmd_.pCommandList);

		// unmap upload vertex
		{
			const auto mapWrittenRange = CD3DX12_RANGE{ 0, nVertices_ * sizeof(Vertex_) };
			currentFrameResource_->pVertexBuffer->Unmap(0, &mapWrittenRange);
			pVertexUpload_ = nullptr;
		}
		// unmap upload index
		{
			const auto mapWrittenRange = CD3DX12_RANGE{ 0, nIndices_ * sizeof(unsigned short) };
			currentFrameResource_->pIndexBuffer->Unmap(0, &mapWrittenRange);
			pIndexUpload_ = nullptr;
		}

		// set pipeline state 
		cmd_.pCommandList->SetPipelineState(pPipelineState_.Get());
		cmd_.pCommandList->SetGraphicsRootSignature(pRootSignature_.Get());
		// configure IA 
		cmd_.pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd_.pCommandList->IASetVertexBuffers(0, 1, &currentFrameResource_->vertexBufferView);
		cmd_.pCommandList->IASetIndexBuffer(&currentFrameResource_->indexBufferView);
		// bind the heap containing the texture descriptor
		{
			ID3D12DescriptorHeap* heapArray[1] = { pSpriteCodex_->GetHeap() };
			cmd_.pCommandList->SetDescriptorHeaps(1, heapArray);
		}
		// bind the descriptor table containing the texture descriptor
		cmd_.pCommandList->SetGraphicsRootDescriptorTable(0, pSpriteCodex_->GetTableHandle());
		// draw vertices
		cmd_.pCommandList->DrawIndexedInstanced(nIndices_, 1, 0, 0, 0);

		// return frame resource to pool
		frameResourcePool_.PutResource(std::move(*currentFrameResource_), frameFenceValue_);
		currentFrameResource_.reset();

		// reliquish command list to be executed on a queue
		return std::move(cmd_);
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
			.SizeInBytes = sizeof(Vertex_) * maxVertices_,
			.StrideInBytes = sizeof(Vertex_),
		};
		// index buffer
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned short) * maxIndices_);
			pDeviceInterface->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&fr.pIndexBuffer)
			) >> chk;
		}
		// index buffer view
		fr.indexBufferView = {
			.BufferLocation = fr.pIndexBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = sizeof(unsigned short) * maxIndices_,
			.Format = DXGI_FORMAT_R16_UINT,
		};

		return fr;
	}



	// sprite frame
	// ------------

	SpriteFrame::SpriteFrame(const spa::RectF& frameInPixels, size_t atlasIndex, std::shared_ptr<SpriteCodex> pCodex)
		:
		atlasIndex_{ atlasIndex },
		pCodex_{ std::move(pCodex) }
	{
		atlasDimensions_ = pCodex_->GetAtlasDimensions(atlasIndex_);
		frameInTexcoords_ = {
			.left = frameInPixels.left / atlasDimensions_.width,
			.top = frameInPixels.top / atlasDimensions_.height,
			.right = frameInPixels.right / atlasDimensions_.width,
			.bottom = frameInPixels.bottom / atlasDimensions_.height,
		};
	}

	SpriteFrame::SpriteFrame(const spa::DimensionsI& cellGridDimensions, const spa::Vec2I& cellCoordinates, size_t atlasIndex, std::shared_ptr<SpriteCodex> pCodex)
		:
		atlasIndex_{ atlasIndex },
		pCodex_{ std::move(pCodex) }
	{
		atlasDimensions_ = pCodex_->GetAtlasDimensions(atlasIndex_);
		const auto cellWidth = atlasDimensions_.width / float(cellGridDimensions.width);
		const auto cellHeight = atlasDimensions_.height / float(cellGridDimensions.height);
		const auto frameInPixels = spa::RectF{
			.left = cellWidth * cellCoordinates.x,
			.top = cellHeight * cellCoordinates.y,
			.right = cellWidth * (cellCoordinates.x + 1),
			.bottom = cellHeight * (cellCoordinates.y + 1),
		};
		frameInTexcoords_ = {
			.left = frameInPixels.left / atlasDimensions_.width,
			.top = frameInPixels.top / atlasDimensions_.height,
			.right = frameInPixels.right / atlasDimensions_.width,
			.bottom = frameInPixels.bottom / atlasDimensions_.height,
		};
	}

	void SpriteFrame::DrawToBatch(ISpriteBatcher& batch, const spa::Vec2F& pos, float rotation, const spa::Vec2F& scale) const
	{
		// deriving dest in pixel coordinates from texcoord source frame and source atlas dimensions
		const auto destPixelDims = frameInTexcoords_.GetDimensions() * atlasDimensions_;
		batch.Draw(atlasIndex_, frameInTexcoords_, destPixelDims, pos, rotation, scale);
	}



	// sprite codex
	// ------------

	SpriteCodex::SpriteCodex(std::shared_ptr<IDevice> pDevice, UINT maxNumAtlases)
		:
		pDevice_{ std::move(pDevice) },
		maxNumAtlases_{ maxNumAtlases }
	{
		// descriptor heap for srvs
		{
			const D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = (UINT)maxNumAtlases,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			};
			pDevice_->GetD3D12DeviceInterface()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&pSrvHeap_)) >> chk;
		}
		// size of descriptors used for index calculation
		descriptorSize_ = pDevice_->GetD3D12DeviceInterface()->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void SpriteCodex::AddSpriteAtlas(std::shared_ptr<ITexture> pTexture)
	{
		chilass(curNumAtlases_ < maxNumAtlases_);

		// get handle to the destination descriptor
		auto descriptorHandle = pSrvHeap_->GetCPUDescriptorHandleForHeapStart();
		descriptorHandle.ptr += SIZE_T(descriptorSize_) * SIZE_T(curNumAtlases_);
		// write into descriptor
		pTexture->WriteDescriptor(pDevice_->GetD3D12DeviceInterface().Get(), descriptorHandle);
		// store in atlas array
		spriteAtlases_.push_back(std::make_unique<SpriteAtlas_>(descriptorHandle, std::move(pTexture)));
		// update number of atlases stored
		curNumAtlases_++;
	}

	ID3D12DescriptorHeap* SpriteCodex::GetHeap() const
	{
		return pSrvHeap_.Get();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE SpriteCodex::GetTableHandle() const
	{
		return pSrvHeap_->GetGPUDescriptorHandleForHeapStart();
	}

	spa::DimensionsI SpriteCodex::GetAtlasDimensions(size_t atlasIndex) const
	{
		chilass(atlasIndex < curNumAtlases_);

		return spriteAtlases_[atlasIndex]->pTexture_->GetDimensions();
	}
}