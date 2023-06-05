#pragma once
#include "Device.h"
#include <Core/src/win/ChilWin.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <deque>
#include "CommandListPair.h"
#include "ICommandQueue.h"

namespace chil::gfx::d12
{
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