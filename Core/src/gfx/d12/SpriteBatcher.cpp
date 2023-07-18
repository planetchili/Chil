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

	SpriteBatcher::SpriteBatcher(std::shared_ptr<IDevice> pDevice)
		:
		pDevice_{ std::move(pDevice) }
	{		
		auto pDeviceInterface = pDevice_->GetD3D12DeviceInterface();
		// root signature
		{
			// define root signature with a matrix of 16 32-bit floats used by the vertex shader (mvp matrix) 
			CD3DX12_ROOT_PARAMETER rootParameters[1]{};
			{
				const CD3DX12_DESCRIPTOR_RANGE descRange{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
				rootParameters[0].InitAsDescriptorTable(1, &descRange);
			}
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
			} pipelineStateStream;

			// define the Vertex input layout 
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

			// building the pipeline state object 
			const D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
				sizeof(PipelineStateStream), &pipelineStateStream
			};
			pDeviceInterface->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pPipelineState_)) >> chk;
		}
		// descriptor heap for srvs
		{
			const D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = 1,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			};
			pDeviceInterface->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&pSrvHeap_)) >> chk;
		}
		// srv descriptor handle
		srvHandle_ = pSrvHeap_->GetCPUDescriptorHandleForHeapStart();
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
		currentFrameResource_->pVertexBuffer_->Map(0,&mapReadRangeNone,
			reinterpret_cast<void**>(&pVertexUpload_)) >> chk;
		// index buffer
		currentFrameResource_->pIndexBuffer_->Map(0, &mapReadRangeNone,
			reinterpret_cast<void**>(&pIndexUpload_)) >> chk;
		// write indices reset
		nVertices_ = 0;
		nIndices_ = 0;
	}
	void SpriteBatcher::SetCamera(const spa::Vec2F& pos, float rot, float scale)
	{
		chilchk_fail;
	}
	void SpriteBatcher::Draw(const spa::RectF& src, const spa::RectF& dest)
	{
		chilass(nVertices_ + 4 <= maxVertices_);
		chilass(nIndices_ + 6 <= maxIndices_);
		// write indices
		pIndexUpload_[nIndices_++] = nVertices_;
		pIndexUpload_[nIndices_++] = nVertices_ + 1;
		pIndexUpload_[nIndices_++] = nVertices_ + 2;
		pIndexUpload_[nIndices_++] = nVertices_ + 1;
		pIndexUpload_[nIndices_++] = nVertices_ + 3;
		pIndexUpload_[nIndices_++] = nVertices_ + 2;
		// write vertices
		pVertexUpload_[nVertices_++] = Vertex_{
			DirectX::XMFLOAT3{ dest.left, dest.top, 0.f },
			DirectX::XMFLOAT2{ src.left, src.top },
		};
		pVertexUpload_[nVertices_++] = Vertex_{
			DirectX::XMFLOAT3{ dest.right, dest.top, 0.f },
			DirectX::XMFLOAT2{ src.right, src.top },
		};
		pVertexUpload_[nVertices_++] = Vertex_{
			DirectX::XMFLOAT3{ dest.left, dest.bottom, 0.f },
			DirectX::XMFLOAT2{ src.left, src.bottom },
		};
		pVertexUpload_[nVertices_++] = Vertex_{
			DirectX::XMFLOAT3{ dest.right, dest.bottom, 0.f },
			DirectX::XMFLOAT2{ src.right, src.bottom },
		};
	}
	CommandListPair SpriteBatcher::EndBatch()
	{
		chilass(cmd_.pCommandAllocator);
		chilass(cmd_.pCommandList);

		// unmap upload vertex
		{
			const auto mapWrittenRange = CD3DX12_RANGE{ 0, nVertices_ * sizeof(Vertex_) };
			currentFrameResource_->pVertexBuffer_->Unmap(0, &mapWrittenRange);
			pVertexUpload_ = nullptr;
		}
		// unmap upload index
		{
			const auto mapWrittenRange = CD3DX12_RANGE{ 0, nIndices_ * sizeof(unsigned short) };
			currentFrameResource_->pIndexBuffer_->Unmap(0, &mapWrittenRange);
			pIndexUpload_ = nullptr;
		}

		// set pipeline state 
		cmd_.pCommandList->SetPipelineState(pPipelineState_.Get());
		cmd_.pCommandList->SetGraphicsRootSignature(pRootSignature_.Get());
		// configure IA 
		cmd_.pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd_.pCommandList->IASetVertexBuffers(0, 1, &currentFrameResource_->vertexBufferView_);
		cmd_.pCommandList->IASetIndexBuffer(&currentFrameResource_->indexBufferView_);
		// bind the heap containing the texture descriptor
		cmd_.pCommandList->SetDescriptorHeaps(1, pSrvHeap_.GetAddressOf());
		// bind the descriptor table containing the texture descriptor
		cmd_.pCommandList->SetGraphicsRootDescriptorTable(0, pSrvHeap_->GetGPUDescriptorHandleForHeapStart());
		// draw vertices
		cmd_.pCommandList->DrawIndexedInstanced(nIndices_, 1, 0, 0, 0);

		// return frame resource to pool
		frameResourcePool_.PutResource(std::move(*currentFrameResource_), frameFenceValue_);
		currentFrameResource_.reset();

		// reliquish command list to be executed on a queue
		return std::move(cmd_);
	}
	void SpriteBatcher::AddTexture(std::shared_ptr<ITexture> pTexture)
	{
		pTexture_ = std::move(pTexture);
		pTexture_->WriteDescriptor(pDevice_->GetD3D12DeviceInterface().Get(), srvHandle_);
	}
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
				nullptr, IID_PPV_ARGS(&fr.pVertexBuffer_)
			) >> chk;
		}
		// vertex buffer view
		fr.vertexBufferView_ = {
			.BufferLocation = fr.pVertexBuffer_->GetGPUVirtualAddress(),
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
				nullptr, IID_PPV_ARGS(&fr.pIndexBuffer_)
			) >> chk;
		}
		// index buffer view
		fr.indexBufferView_ = {
			.BufferLocation = fr.pIndexBuffer_->GetGPUVirtualAddress(),
			.SizeInBytes = sizeof(unsigned short) * maxIndices_,
			.Format = DXGI_FORMAT_R16_UINT,
		};

		return fr;
	}
}