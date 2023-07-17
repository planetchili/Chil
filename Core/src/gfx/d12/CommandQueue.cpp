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
		pDeviceInterface->CreateFence(currentFenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence_)) >> chk;
	}
	CommandListPair CommandQueue::GetCommandListPair()
	{
		std::lock_guard lk{ mutex_ };

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
	void CommandQueue::ExecuteCommandList(CommandListPair commandListPair)
	{
		std::lock_guard lk{ mutex_ };

		// close the command list
		commandListPair.pCommandList->Close() >> chk;
		// execute the list
		{
			ID3D12CommandList* const ppCommandLists[]{
				commandListPair.pCommandList.Get()
			};
			pCommandQueue_->ExecuteCommandLists(1, ppCommandLists);
		}
		// queue allocator to the queue for eventual re-use
		// specific fence value not requested/returned so just manage at frame granularity
		commandAllocatorQueue_.push_back({ frameFenceValue_, std::move(commandListPair.pCommandAllocator) });
		// return command list to pool
		commandListPool_.push_back(std::move(commandListPair.pCommandList));
	}
	uint64_t CommandQueue::ExecuteCommandListWithFence(CommandListPair commandListPair)
	{
		std::lock_guard lk{ mutex_ };

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
		pCommandQueue_->Signal(pFence_.Get(), ++currentFenceValue_) >> chk;
		const auto fenceValue = currentFenceValue_.load();
		// queue allocator to the queue for eventual re-use
		commandAllocatorQueue_.push_back({ fenceValue, std::move(commandListPair.pCommandAllocator) });
		// return command list to pool
		commandListPool_.push_back(std::move(commandListPair.pCommandList));
		// return the fence value corresponding to completion of this command list
		return fenceValue;
	}
	uint64_t CommandQueue::SignalFence()
	{
		std::lock_guard lk{ mutex_ };

		pCommandQueue_->Signal(pFence_.Get(), ++currentFenceValue_) >> chk;
		return currentFenceValue_;
	}
	uint64_t CommandQueue::SignalFrameFence()
	{
		std::lock_guard lk{ mutex_ };

		currentFenceValue_ = frameFenceValue_.load();
		pCommandQueue_->Signal(pFence_.Get(), currentFenceValue_) >> chk;
		frameFenceValue_ += maxFencesPerFrame_;
		return currentFenceValue_;
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
	uint64_t CommandQueue::GetFrameFenceValue() const
	{
		return frameFenceValue_;
	}
	uint64_t CommandQueue::GetSignalledFenceValue() const
	{
		return pFence_->GetCompletedValue();
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