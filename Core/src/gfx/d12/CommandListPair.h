#pragma once
#include "WrapD3D.h"
#include <wrl/client.h>

namespace chil::gfx::d12
{
	struct CommandListPair
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList;
	};
}