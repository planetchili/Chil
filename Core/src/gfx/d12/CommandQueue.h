#pragma once
#include "Device.h"
#include <Core/src/win/ChilWin.h>
#include <d3d12.h> 
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <deque>

namespace chil::gfx::d12
{
	struct CommandListPair
	{
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator;
	};

	class ICommandQueue
	{
	public:
		virtual ~ICommandQueue() = default;
		virtual CommandListPair GetCommandListPair() = 0;
		virtual uint64_t ExecuteCommandList(CommandListPair commandListPair) = 0;
		virtual uint64_t SignalFence() = 0;
		virtual bool FenceHasReached(uint64_t fenceValue) const = 0;
		virtual void WaitForFenceValue(uint64_t fenceValue) const = 0;
		virtual void Flush() = 0;
		virtual Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const = 0;
	};

	class CommandQueue : public ICommandQueue
	{
	public:
		CommandQueue(std::shared_ptr<IDevice> pDevice,
			D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT);
		CommandListPair GetCommandListPair() override;
		uint64_t ExecuteCommandList(CommandListPair commandListPair) override;
		uint64_t SignalFence() override;
		bool FenceHasReached(uint64_t fenceValue) const override;
		void WaitForFenceValue(uint64_t fenceValue) const override;
		void Flush() override;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const override;
	protected:
		virtual Microsoft::WRL::ComPtr<ID3D12CommandAllocator> MakeCommandAllocator();
		virtual Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> MakeCommandList(
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator);
	private:
		// types
		struct CommandAllocatorEntry_
		{
			uint64_t fenceValue;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator;
		};
		// data
		D3D12_COMMAND_LIST_TYPE commandListType_;
		std::shared_ptr<IDevice> pDevice_;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> pCommandQueue_;
		Microsoft::WRL::ComPtr<ID3D12Fence> pFence_;
		uint64_t fenceValue_ = 0;
		std::deque<CommandAllocatorEntry_> commandAllocatorQueue_;
		std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> commandListPool_;
	};
}