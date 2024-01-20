#pragma once
#include "Device.h"
#include "WrapD3D.h"
#include <wrl/client.h>
#include "CommandListPair.h"
#include "ICommandQueue.h"
#include <memory>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>

namespace chil::gfx::d12
{
	class CommandQueue : public ICommandQueue
	{
	public:
		CommandQueue(std::shared_ptr<IDevice> pDevice,
			D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT);
		CommandListPair GetCommandListPair() override;
		void ExecuteCommandList(CommandListPair commandListPair) override;
		uint64_t ExecuteCommandListWithFence(CommandListPair commandListPair) override;
		uint64_t SignalFence() override;
		uint64_t SignalFrameFence() override;
		bool FenceHasReached(uint64_t fenceValue) const override;
		void WaitForFenceValue(uint64_t fenceValue) const override;
		// get the value to be signalled on the fence when current frame completes
		uint64_t GetFrameFenceValue() const override;
		// get actual value signalled on the fence (via command list completion etc.)
		uint64_t GetSignalledFenceValue() const override;
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
		static constexpr uint64_t maxFencesPerFrame_ = 100;
		D3D12_COMMAND_LIST_TYPE commandListType_;
		std::shared_ptr<IDevice> pDevice_;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> pCommandQueue_;
		Microsoft::WRL::ComPtr<ID3D12Fence> pFence_;
		std::atomic<uint64_t> currentFenceValue_ = 0;
		std::atomic<uint64_t> frameFenceValue_ = maxFencesPerFrame_;
		std::deque<CommandAllocatorEntry_> commandAllocatorQueue_;
		std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> commandListPool_;
		std::mutex mutex_;
	};
}