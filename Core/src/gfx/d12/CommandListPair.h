#pragma once
#include <Core/src/win/ChilWin.h>
#include <d3d12.h> 
#include <wrl/client.h>

namespace chil::gfx::d12
{
	struct CommandListPair
	{
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator;
	};
}