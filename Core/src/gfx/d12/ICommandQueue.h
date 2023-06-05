#pragma once
#include <Core/src/win/ChilWin.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include "CommandListPair.h"

namespace chil::gfx::d12
{
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
}