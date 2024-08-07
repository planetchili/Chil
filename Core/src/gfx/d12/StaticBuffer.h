#pragma once
#include "WrapD3D.h"
#include "Device.h"
#include <span>
#include <wrl/client.h>
#include "CommandListPair.h"
#include <limits>
#include <iterator>
#include <vector>

namespace chil::gfx::d12
{
	class StaticBufferBase_
	{
	public:
		template<typename T>
		StaticBufferBase_(IDevice& device, const std::vector<T>& data)
			:
			StaticBufferBase_{ device, sizeof(T), data.size()}
		{
			FillUploadBuffer(data);
		}
		template<typename T>
		void FillUploadBuffer(const std::vector<T>& data)
		{
			FillUploadBuffer_(std::span{ reinterpret_cast<const char*>(data.data()), data.size() * sizeof(T) });
		}
		bool UploadScheduled() const;
		bool UploadComplete(uint64_t currentSignalledFenceValue) const;
		void CollectGarbage(uint64_t currentSignalledFenceValue);
	protected:
		void WriteCopyCommands_(CommandListPair& cmd, D3D12_RESOURCE_STATES endState, uint64_t frameFenceValue);
		template<typename V>
		void InitializeView_(V& view) const
		{
			if constexpr (std::same_as<V, D3D12_BUFFER_SRV>) {
				auto& v = static_cast<D3D12_BUFFER_SRV&>(view);
				v.NumElements = numElements_;
				v.FirstElement = 0;
				v.StructureByteStride = strideInBytes_;
				v.Flags = D3D12_BUFFER_SRV_FLAGS(0);
			}
			else {
				view.BufferLocation = pBuffer_->GetGPUVirtualAddress();
				view.SizeInBytes = (UINT)GetTotalSize_();
			}
		}
		ID3D12Resource* GetResource_() const;
	private:
		// functions
		void FillUploadBuffer_(std::span<const char>);
		StaticBufferBase_(IDevice& device, size_t strideInBytes, size_t numElements);
		size_t GetTotalSize_() const;
		// data
		static constexpr uint64_t emptyFenceValue = std::numeric_limits<uint64_t>::max();
		Microsoft::WRL::ComPtr<ID3D12Resource> pBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer_;
		uint64_t uploadCompletionFrameFenceValue_ = emptyFenceValue;
		uint32_t strideInBytes_;
		uint32_t numElements_;
	};

	template<typename V>
	concept ConCpuBufferView = std::same_as<V, D3D12_VERTEX_BUFFER_VIEW> ||
		std::same_as<V, D3D12_INDEX_BUFFER_VIEW>;

	template<ConCpuBufferView ViewType>
	class StaticCpuBuffer : public StaticBufferBase_
	{
	public:
		template<typename T>
		StaticCpuBuffer(IDevice& device, const std::vector<T>& data)
			:
			StaticBufferBase_{ device, data }
		{
			InitializeView_(view);
			if constexpr (std::same_as<ViewType, D3D12_INDEX_BUFFER_VIEW>) {
				static_assert(std::unsigned_integral<T> && (sizeof(T) == 2 || sizeof(T) == 4));
				view.Format = sizeof(T) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
			}
			else if constexpr (std::same_as<ViewType, D3D12_VERTEX_BUFFER_VIEW>) {
				view.StrideInBytes = sizeof(T);
			}
		}
		void WriteCopyCommands(CommandListPair& cmd, uint64_t frameFenceValue)
		{
			if constexpr (std::same_as<ViewType, D3D12_INDEX_BUFFER_VIEW>) {
				WriteCopyCommands_(cmd, D3D12_RESOURCE_STATE_INDEX_BUFFER, frameFenceValue);
			}
			else if constexpr (std::same_as<ViewType, D3D12_VERTEX_BUFFER_VIEW>) {
				WriteCopyCommands_(cmd, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, frameFenceValue);
			}
		}
		const ViewType& GetView() const
		{
			return view;
		}
	private:
		ViewType view;
	};

	class StaticConstantBuffer : public StaticBufferBase_
	{
	public:
		using StaticBufferBase_::StaticBufferBase_;
		void WriteCopyCommands(CommandListPair& cmd, uint64_t frameFenceValue);
		void WriteDescriptor(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle) const;
		D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const;
	};

	class StructuredBuffer : public StaticBufferBase_
	{
	public:
		using StaticBufferBase_::StaticBufferBase_;
		void WriteCopyCommands(CommandListPair& cmd, uint64_t frameFenceValue);
		void WriteDescriptor(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle) const;
		D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const;
	};
}