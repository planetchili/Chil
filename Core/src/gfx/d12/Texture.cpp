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
		// create the cube texture
		ComPtr<ID3D12Resource> cubeFaceTexture;

		// load image data from disk
		DirectX::ScratchImage image;
		DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image) >> chk;

		// generate mip chain
		DirectX::ScratchImage mipChain;
		DirectX::GenerateMipMaps(*image.GetImages(), DirectX::TEX_FILTER_BOX, 0, mipChain) >> chk;

		// create texture resource
		{
			const auto& chainBase = *mipChain.GetImages();
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
				IID_PPV_ARGS(&cubeFaceTexture)
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
		ComPtr<ID3D12Resource> uploadBuffer;
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto uploadBufferSize = GetRequiredIntermediateSize(
				cubeFaceTexture.Get(), 0, (UINT)subresourceData.size()
			);
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadBuffer)
			) >> chk;
		}

		// write commands to copy data to upload texture (copying each subresource)
		UpdateSubresources(
			cmd.pCommandList.Get(),
			cubeFaceTexture.Get(),
			uploadBuffer.Get(),
			0, 0,
			(UINT)subresourceData.size(),
			subresourceData.data()
		);
		// write command to transition texture to texture state 
		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				cubeFaceTexture.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			cmd.pCommandList->ResourceBarrier(1, &barrier);
		}
	}
	Texture::~Texture() = default;
}