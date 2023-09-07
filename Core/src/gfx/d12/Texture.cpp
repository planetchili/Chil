#pragma once
#include "Texture.h"
#include <Core/src/utl/HrChecker.h>
#include <Core/src/log/Log.h>
#include "DirectXTex.h"
#include <ranges>

#pragma warning(push)
#pragma warning(disable : 26495)
#include "d3dx12.h"
#pragma warning(pop)


namespace chil::gfx::d12
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;
	namespace rn = std::ranges;
	namespace vi = rn::views;

	Texture::Texture(ComPtr<ID3D12Device2> pDevice, CommandListPair cmd, std::wstring path)
	{
		// load image data from disk
		DirectX::ScratchImage image;
		DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image) >> chk;

		// generate mip chain
		DirectX::ScratchImage mipChain;
		DirectX::GenerateMipMaps(*image.GetImages(), DirectX::TEX_FILTER_BOX, 0, mipChain) >> chk;

		// create texture resource
		{
			const auto& chainBase = *mipChain.GetImages();
			dimensions_ = { int(chainBase.width), int(chainBase.height) };
			const D3D12_RESOURCE_DESC texDesc{
				.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.Width = (UINT)chainBase.width,
				.Height = (UINT)chainBase.height,
				.DepthOrArraySize = 1,
				.MipLevels = (UINT16)mipChain.GetImageCount(),
				.Format = chainBase.format,
				.SampleDesc = {.Count = 1 },
				.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
				.Flags = D3D12_RESOURCE_FLAG_NONE,
			};
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&pResource_)
			) >> chk;
		}

		// collect subresource data
		const auto subresourceData = vi::iota(0, (int)mipChain.GetImageCount()) |
			vi::transform([&](int i) {
				const auto img = mipChain.GetImage(i, 0, 0);
				return D3D12_SUBRESOURCE_DATA{
					.pData = img->pixels,
					.RowPitch = (LONG_PTR)img->rowPitch,
					.SlicePitch = (LONG_PTR)img->slicePitch,
				};
			}) |
			rn::to<std::vector>();

		// create the intermediate upload buffer
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto uploadBufferSize = GetRequiredIntermediateSize(
				pResource_.Get(), 0, (UINT)subresourceData.size()
			);
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&pIntermediate_)
			) >> chk;
		}

		// write commands to copy data to upload texture (copying each subresource)
		UpdateSubresources(
			cmd.pCommandList.Get(),
			pResource_.Get(),
			pIntermediate_.Get(),
			0, 0,
			(UINT)subresourceData.size(),
			subresourceData.data()
		);
		// write command to transition texture to texture state 
		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				pResource_.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			cmd.pCommandList->ResourceBarrier(1, &barrier);
		}
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
	spa::DimensionsI Texture::GetDimensions() const
	{
		return dimensions_;
	}
	Texture::~Texture() = default;
}