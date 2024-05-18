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

	StaticBufferBase_::StaticBufferBase_(IDevice& device, size_t sizeInBytes)
		:
		sizeInBytes_{ (uint32_t)sizeInBytes }
	{
		auto pDeviceInterface = device.GetD3D12DeviceInterface();
		// create static buffer for use with rendering
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes_);
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
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes_);
			pDeviceInterface->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&pUploadBuffer_)
			) >> chk;
		}
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

	void StaticBufferBase_::FillUploadBuffer_(std::span<const char> data)
	{
		assert(sizeInBytes_ == data.size());
		char* mappedIndexData = nullptr;
		pUploadBuffer_->Map(0, &nullRange, reinterpret_cast<void**>(&mappedIndexData)) >> chk;
		rn::copy(data, mappedIndexData);
		D3D12_RANGE writtenRange{ 0, data.size() };
		pUploadBuffer_->Unmap(0, &writtenRange);
	}
	void StaticConstantBuffer::WriteDescriptor(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle) const
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		InitializeView_(cbvDesc);
		pDevice->CreateConstantBufferView(&cbvDesc, handle);
	}
	auto StaticConstantBuffer::GetGpuAddress() const
	{
		return GetGpuAddress_();
	}
}