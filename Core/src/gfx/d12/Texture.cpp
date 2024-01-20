#pragma once
#include "Texture.h"
#include <Core/src/utl/HrChecker.h>
#include <Core/src/log/Log.h>
#include <Core/src/utl/Assert.h>
#include "WrapD3DX.h"


namespace chil::gfx::d12
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;

	Texture::Texture(IocParams&& params)
		:
		dimensions_{ params.pixelDimensions }
	{
		// create texture resource
		const D3D12_RESOURCE_DESC texDesc{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Width = (UINT)params.pixelDimensions.width,
			.Height = (UINT)params.pixelDimensions.height,
			.DepthOrArraySize = 1,
			.MipLevels = params.mipLevels,
			.Format = params.format,
			.SampleDesc = {.Count = 1 },
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAG_NONE,
		};
		const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
		params.pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&pResource_)
		) >> chk;
	}
	void Texture::WriteDescriptor(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle) const
	{
		const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format = pResource_->GetDesc().Format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D{.MipLevels = pResource_->GetDesc().MipLevels },
		};
		pDevice->CreateShaderResourceView(pResource_.Get(), &srvDesc, handle);
	}
	ID3D12Resource& Texture::GetResource()
	{
		chilass(pResource_);
		return *pResource_.Get();
	}
	spa::DimensionsI Texture::GetDimensions() const
	{
		return dimensions_;
	}
}