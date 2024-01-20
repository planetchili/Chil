#pragma once
#include <Core/src/spa/Dimensions.h>
#include "WrapD3D.h"
#include <wrl/client.h>
#include "CommandListPair.h"
#include "../ITexture.h"
#include <string>

namespace chil::gfx::d12
{
	class ITexture : public gfx::ITexture
	{
	public:
		// types
		struct IocParams
		{
			spa::DimensionsI pixelDimensions;
			uint16_t mipLevels;
			DXGI_FORMAT format;
			ID3D12Device2* pDevice;
		};
		// functions
		virtual void WriteDescriptor(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle) const = 0;
		virtual ID3D12Resource& GetResource() = 0;
	};

	class Texture : public ITexture
	{
	public:
		Texture(IocParams&& params);
		void WriteDescriptor(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle) const override;
		ID3D12Resource& GetResource() override;
		spa::DimensionsI GetDimensions() const override;
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> pResource_;
		spa::DimensionsI dimensions_;
	};
}
