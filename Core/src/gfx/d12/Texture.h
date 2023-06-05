#pragma once
#include <Core/src/win/ChilWin.h>
#include <d3d12.h> 
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "CommandListPair.h"
#include <string>

namespace chil::gfx::d12
{
	class ITexture
	{
	public:
		virtual ~ITexture() = default;
	};

	class Texture : public ITexture
	{
	public:
		Texture(Microsoft::WRL::ComPtr<ID3D12Device2> pDevice, CommandListPair cmd, std::wstring path);
		~Texture() override;
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> pResource_;
	};
}