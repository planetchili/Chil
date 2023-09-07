#pragma once
#include <Core/src/win/ChilWin.h>
#include <Core/src/spa/Dimensions.h>
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
		virtual void WriteDescriptor(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle) const = 0;
		virtual spa::DimensionsI GetDimensions() const = 0;
	};

	class Texture : public ITexture
	{
	public:
		Texture(Microsoft::WRL::ComPtr<ID3D12Device2> pDevice, CommandListPair cmd, std::wstring path);
		void ClearIntermediate() { pIntermediate_.Reset(); }
		void WriteDescriptor(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle) const override;
		spa::DimensionsI GetDimensions() const override;
		~Texture() override;
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> pResource_;
		spa::DimensionsI dimensions_;
		Microsoft::WRL::ComPtr<ID3D12Resource> pIntermediate_;
	};
}
