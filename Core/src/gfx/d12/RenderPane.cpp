#include "RenderPane.h"
#include <Core/src/utl/HrChecker.h>
#include <Core/src/log/Log.h>
#include <Core/src/utl/String.h>
#pragma warning(push)
#pragma warning(disable : 26495)
#include "d3dx12.h"
#pragma warning(pop)
#include "ResourceLoader.h"
#include <d3dcompiler.h>
#include <ranges>

namespace chil::gfx::d12
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;
	namespace rn = std::ranges;

	RenderPane::RenderPane(HWND hWnd, const spa::DimensionsI& dims, std::shared_ptr<IDevice> pDevice,
		std::shared_ptr<ICommandQueue> pCommandQueue)
		:
		dims_{ dims },
		pDevice_{ std::move(pDevice) },
		pCommandQueue_{ std::move(pCommandQueue) }
	{
		// cache device interface
		auto pDeviceInterface = pDevice_->GetD3D12DeviceInterface();
		// create swap chain
		{
			const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
				.Width = (UINT)dims.width,
				.Height = (UINT)dims.height,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.Stereo = FALSE,
				.SampleDesc = {
					.Count = 1,
					.Quality = 0
				},
				.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
				.BufferCount = bufferCount_,
				.Scaling = DXGI_SCALING_STRETCH,
				.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
				.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
				.Flags = 0,
			};
			ComPtr<IDXGISwapChain1> swapChain1;
			pDevice_->GetDXGIFactoryInterface()->CreateSwapChainForHwnd(
				pCommandQueue_->GetD3D12CommandQueue().Get(),
				hWnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain1) >> chk;
			swapChain1.As(&pSwapChain_) >> chk;
		}
		// create rtv descriptor heap
		{
			const D3D12_DESCRIPTOR_HEAP_DESC desc = {
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
				.NumDescriptors = bufferCount_,
			};
			pDeviceInterface->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pRtvDescriptorHeap_)) >> chk;
		}
		// cache rtv descriptor size
		rtvDescriptorSize_ = pDeviceInterface->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		// create rtvs and get resource handles for each buffer in the swap chain
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
				pRtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
			for (int i = 0; i < bufferCount_; i++) {
				pSwapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i])) >> chk;
				pDeviceInterface->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(rtvDescriptorSize_);
			}
		}

		// sprote-mein
		// vertex buffer
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * maxVertices_);
			pDeviceInterface->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&pVertexBuffer_)
			) >> chk;
		}
		// vertex buffer view
		vertexBufferView_ = {
			.BufferLocation = pVertexBuffer_->GetGPUVirtualAddress(),
			.SizeInBytes = sizeof(Vertex) * maxVertices_,
			.StrideInBytes = sizeof(Vertex),
		};
		// texture
		{
			ResourceLoader loader{ pDevice_ };
			pTexture_ = loader.LoadTexture(L"sprote-shiet.png").get();
		}
		// srv heap
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
		// create descriptor in the heap
		pTexture_->WriteDescriptor(pDevice_->GetD3D12DeviceInterface().Get(), srvHandle_);
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
			const CD3DX12_STATIC_SAMPLER_DESC staticSampler{ 0, D3D12_FILTER_MIN_MAG_MIP_LINEAR };
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
		// scissor rect
		scissorRect_ = CD3DX12_RECT{ 0, 0, LONG_MAX, LONG_MAX };
		// viewport
		viewport_ = CD3DX12_VIEWPORT{ 0.0f, 0.0f, float(dims_.width), float(dims_.height) };
	}

	RenderPane::~RenderPane()
	{
		// wait for queue to become completely empty
		pCommandQueue_->Flush();
	}

	void RenderPane::BeginFrame()
	{
		// set index of swap chain buffer for this frame
		curBackBufferIndex_ = pSwapChain_->GetCurrentBackBufferIndex();
		// wait for this back buffer to become free
		pCommandQueue_->WaitForFenceValue(bufferFenceValues_[curBackBufferIndex_]);
		// acquire command list
		commandListPair_ = pCommandQueue_->GetCommandListPair();
		// transition buffer resource to render target state 
		auto& backBuffer = backBuffers_[curBackBufferIndex_];
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandListPair_.pCommandList->ResourceBarrier(1, &barrier);
	}

	void RenderPane::EndFrame()
	{
		auto& backBuffer = backBuffers_[curBackBufferIndex_];

		// $$$$ sprote funsies $$$$
		// ### copy verts to buffer
		const Vertex verts[]{
			{ { 0.f, 0.5f, 0.f },    { 0.f, 0.f }  },
			{ { 0.5f, -0.5f, 0.f },  { 1.f, 1.f }  },
			{ { -0.5f, -0.5f, 0.f }, { 0.f, 1.f }  },
		};
		{
			Vertex* pDest = nullptr;
			pVertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&pDest)) >> chk;
			rn::copy(verts, pDest);
			pVertexBuffer_->Unmap(0, nullptr);
		}
		// ### bind all the things
		const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv{
			pRtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
			(INT)curBackBufferIndex_, rtvDescriptorSize_ };
		// set pipeline state 
		commandListPair_.pCommandList->SetPipelineState(pPipelineState_.Get());
		commandListPair_.pCommandList->SetGraphicsRootSignature(pRootSignature_.Get());
		// configure IA 
		commandListPair_.pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandListPair_.pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
		// configure RS 
		commandListPair_.pCommandList->RSSetViewports(1, &viewport_);
		commandListPair_.pCommandList->RSSetScissorRects(1, &scissorRect_);
		// bind render target and depth
		commandListPair_.pCommandList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
		// bind the heap containing the texture descriptor
		commandListPair_.pCommandList->SetDescriptorHeaps(1, pSrvHeap_.GetAddressOf());
		// bind the descriptor table containing the texture descriptor
		commandListPair_.pCommandList->SetGraphicsRootDescriptorTable(0, pSrvHeap_->GetGPUDescriptorHandleForHeapStart());
		// ### draw me daddy
		commandListPair_.pCommandList->DrawInstanced(3, 1, 0, 0);

		// prepare buffer for presentation by transitioning to present state
		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				backBuffer.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			commandListPair_.pCommandList->ResourceBarrier(1, &barrier);
		}
		// submit command list 
		pCommandQueue_->ExecuteCommandList(std::move(commandListPair_));
		// present frame 
		pSwapChain_->Present(1, 0) >> chk;

		// $$ making sure we don't use vertex buffer before it's done with
		pCommandQueue_->Flush();

		// insert a fence so we know when the buffer is free
		bufferFenceValues_[curBackBufferIndex_] = pCommandQueue_->SignalFence();
	}

	void RenderPane::Clear(const std::array<float, 4>& color)
	{
		auto& backBuffer = backBuffers_[curBackBufferIndex_];
		const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv{
			pRtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
			(INT)curBackBufferIndex_, rtvDescriptorSize_ };
		commandListPair_.pCommandList->ClearRenderTargetView(rtv, color.data(), 0, nullptr);
	}
}
