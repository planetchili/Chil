#pragma once
#include "StaticBuffer.h"
#include "WrapD3DX.h"
#include <Core/src/utl/HrChecker.h>
#include "Util.h"
#include <ranges>
#include <cassert>

namespace chil::gfx::d12
{
	using utl::chk;
	namespace rn = std::ranges;

	StaticBufferBase_::StaticBufferBase_(IDevice& device, size_t strideInBytes, size_t numElements)
		:
		strideInBytes_{ (uint32_t)strideInBytes },
		numElements_{ (uint32_t)numElements }
	{
		auto pDeviceInterface = device.GetD3D12DeviceInterface();
		// create static buffer for use with rendering
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(GetTotalSize_());
			pDeviceInterface->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr, IID_PPV_ARGS(&pBuffer_)
			) >> chk;
		}
		// create committed resource for cpu upload of index data
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(GetTotalSize_());
			pDeviceInterface->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&pUploadBuffer_)
			) >> chk;
		}
	}

	size_t StaticBufferBase_::GetTotalSize_() const
	{
		return size_t(strideInBytes_) * size_t(numElements_);
	}

	bool StaticBufferBase_::UploadScheduled() const
	{
		return uploadCompletionFrameFenceValue_ != emptyFenceValue;
	}

	bool StaticBufferBase_::UploadComplete(uint64_t currentSignalledFenceValue) const
	{
		return UploadScheduled() && currentSignalledFenceValue >= uploadCompletionFrameFenceValue_;
	}

	void StaticBufferBase_::CollectGarbage(uint64_t currentSignalledFenceValue)
	{
		assert(UploadComplete(currentSignalledFenceValue));
		pUploadBuffer_.Reset();
	}

	void StaticBufferBase_::WriteCopyCommands_(CommandListPair& cmd, D3D12_RESOURCE_STATES endState, uint64_t frameFenceValue)
	{
		// copy upload buffer to vertex buffer  
		cmd.pCommandList->CopyResource(pBuffer_.Get(), pUploadBuffer_.Get());
		// transition vertex buffer to vertex buffer state 
		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				pBuffer_.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, endState);
			cmd.pCommandList->ResourceBarrier(1, &barrier);
		}
		// record completion fence value
		uploadCompletionFrameFenceValue_ = frameFenceValue;
	}

	ID3D12Resource* StaticBufferBase_::GetResource_() const
	{
		return pBuffer_.Get();
	}

	void StaticBufferBase_::FillUploadBuffer_(std::span<const char> data)
	{
		assert(GetTotalSize_() == data.size());
		char* mappedIndexData = nullptr;
		pUploadBuffer_->Map(0, &nullRange, reinterpret_cast<void**>(&mappedIndexData)) >> chk;
		rn::copy(data, mappedIndexData);
		D3D12_RANGE writtenRange{ 0, data.size() };
		pUploadBuffer_->Unmap(0, &writtenRange);
	}
	void StaticConstantBuffer::WriteCopyCommands(CommandListPair& cmd, uint64_t frameFenceValue)
	{
		WriteCopyCommands_(cmd, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, frameFenceValue);
	}
	void StaticConstantBuffer::WriteDescriptor(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle) const
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		InitializeView_(cbvDesc);
		pDevice->CreateConstantBufferView(&cbvDesc, handle);
	}
	D3D12_GPU_VIRTUAL_ADDRESS StaticConstantBuffer::GetGpuAddress() const
	{
		return GetResource_()->GetGPUVirtualAddress();
	}
	void StructuredBuffer::WriteCopyCommands(CommandListPair& cmd, uint64_t frameFenceValue)
	{
		WriteCopyCommands_(cmd, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, frameFenceValue);
	}
	void StructuredBuffer::WriteDescriptor(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle) const
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		InitializeView_(srvDesc.Buffer);
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		pDevice->CreateShaderResourceView(GetResource_(), &srvDesc, handle);
	}
	D3D12_GPU_VIRTUAL_ADDRESS StructuredBuffer::GetGpuAddress() const
	{
		return GetResource_()->GetGPUVirtualAddress();
	}
}