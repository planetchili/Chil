#include "CommandQueue.h"
#include <Core/src/utl/HrChecker.h>
#include <Core/src/log/Log.h>
#pragma warning(push)
#pragma warning(disable : 26495)
#include "d3dx12.h"
#pragma warning(pop)

namespace chil::gfx::d12
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;

	CommandQueue::CommandQueue(std::shared_ptr<IDevice> pDevice, D3D12_COMMAND_LIST_TYPE commandListType)
		:
		pDevice_{ std::move(pDevice) },
		commandListType_{ commandListType }
	{
		auto pDeviceInterface = pDevice_->GetD3D12DeviceInterface();
		const D3D12_COMMAND_QUEUE_DESC desc{ .Type = commandListType_ };
		pDeviceInterface->CreateCommandQueue(&desc, IID_PPV_ARGS(&pCommandQueue_)) >> chk;
		pDeviceInterface->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence_)) >> chk;
	}
	CommandListPair CommandQueue::GetCommandListPair()
	{
		CommandListPair pair;

		// if command allocator at front of queue is free, use it
		if (!commandAllocatorQueue_.empty() && FenceHasReached(commandAllocatorQueue_.front().fenceValue)) {
			pair.pCommandAllocator = std::move(commandAllocatorQueue_.front().pCommandAllocator);
			commandAllocatorQueue_.pop_front();
			// command allocator needs reset before reuse
			pair.pCommandAllocator->Reset() >> chk;
		}
		// otherwise create a new command allocator
		else
		{
			pair.pCommandAllocator = MakeCommandAllocator();
		}

		// if there is a command list in the pool, use it
		if (!commandListPool_.empty())
		{
			pair.pCommandList = std::move(commandListPool_.back());
			commandListPool_.pop_back();
			// command list is pushed in closed state, needs reset
			pair.pCommandList->Reset(pair.pCommandAllocator.Get(), nullptr) >> chk;
		}
		// otherwise create a new command list
		else
		{
			pair.pCommandList = MakeCommandList(pair.pCommandAllocator);
		}
		// return the matched pair of command list and allocator
		return pair;
	}
	uint64_t CommandQueue::ExecuteCommandList(CommandListPair commandListPair)
	{
		// close the command list
		commandListPair.pCommandList->Close() >> chk;
		// execute the list
		{
			ID3D12CommandList* const ppCommandLists[]{
				commandListPair.pCommandList.Get()
			};
			pCommandQueue_->ExecuteCommandLists(1, ppCommandLists);
		}
		// insert signal into queue after list completion
		const auto fenceValue = SignalFence();
		// queue allocator to the queue for eventual re-use
		commandAllocatorQueue_.push_back({ fenceValue, std::move(commandListPair.pCommandAllocator) });
		// return command list to pool
		commandListPool_.push_back(std::move(commandListPair.pCommandList));
		// return the fence value corresponding to completion of this command list
		return fenceValue;
	}
	uint64_t CommandQueue::SignalFence()
	{
		pCommandQueue_->Signal(pFence_.Get(), ++fenceValue_) >> chk;
		return fenceValue_;
	}
	bool CommandQueue::FenceHasReached(uint64_t fenceValue) const
	{
		return pFence_->GetCompletedValue() >= fenceValue;
	}
	void CommandQueue::WaitForFenceValue(uint64_t fenceValue) const
	{
		if (!FenceHasReached(fenceValue)) {
			pFence_->SetEventOnCompletion(fenceValue, nullptr) >> chk;
		}
	}
	void CommandQueue::Flush()
	{
		WaitForFenceValue(SignalFence());
	}
	ComPtr<ID3D12CommandQueue> CommandQueue::GetD3D12CommandQueue() const
	{
		return pCommandQueue_;
	}
	ComPtr<ID3D12CommandAllocator> CommandQueue::MakeCommandAllocator()
	{
		ComPtr<ID3D12CommandAllocator> pCommandAllocator;
		pDevice_->GetD3D12DeviceInterface()->CreateCommandAllocator(
			commandListType_, IID_PPV_ARGS(&pCommandAllocator)) >> chk;
		return pCommandAllocator;
	}
	ComPtr<ID3D12GraphicsCommandList2> CommandQueue::MakeCommandList(ComPtr<ID3D12CommandAllocator> pCommandAllocator)
	{
		ComPtr<ID3D12GraphicsCommandList2> pCommandList;
		pDevice_->GetD3D12DeviceInterface()->CreateCommandList(0,
			commandListType_, pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&pCommandList)) >> chk;
		return pCommandList;
	}
}