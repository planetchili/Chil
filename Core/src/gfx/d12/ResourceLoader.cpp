#include "ResourceLoader.h"
#include "CommandQueue.h"
#include <Core/src/utl/HrChecker.h>
#include <Core/src/log/Log.h>
#include <Core/src/utl/Assert.h>
#include "WrapD3DX.h"
#include "DirectXTex.h"
#include <ranges>

namespace chil::gfx::d12
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;
	namespace rn = std::ranges;
	namespace vi = rn::views;

	ResourceLoader::ResourceLoader(std::shared_ptr<gfx::IDevice> pDevice, TextureFactory textureFactory)
		:
		pDevice_{ std::dynamic_pointer_cast<decltype(pDevice_)::element_type>(std::move(pDevice)) },
		// TODO: inject this to enable IoC
		pQueue_{ std::make_shared<CommandQueue>(pDevice_) },
		textureFactory_{ std::move(textureFactory) }
	{}

	std::future<std::shared_ptr<gfx::ITexture>> ResourceLoader::LoadTexture(std::wstring path)
	{
		// - consider batching multiple loads together in a single command list
		// - consider reusing intermediates / allocating from heap instead of committed
		// - consider using copy / compute queue (still need to transition resource in a graphics queue before use)
		return std::async([=]() mutable {
			// load texture image from disk
			DirectX::ScratchImage image;
			DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image) >> chk;
			// generate mip chain
			DirectX::ScratchImage mipChain;
			DirectX::GenerateMipMaps(*image.GetImages(), DirectX::TEX_FILTER_BOX, 0, mipChain) >> chk;
			// collect subresource data
			const auto subresourceData = vi::iota(0, (int)mipChain.GetImageCount()) |
				vi::transform([&](int i) {
				const auto img = mipChain.GetImage(i, 0, 0);
				return D3D12_SUBRESOURCE_DATA{
					.pData = img->pixels,
					.RowPitch = (LONG_PTR)img->rowPitch,
					.SlicePitch = (LONG_PTR)img->slicePitch,
				};
			}) | rn::to<std::vector>();
			// create texture object
			const auto& chainBase = *mipChain.GetImages();
			d12::ITexture::IoCParams texParams{
				.pixelDimensions = { (int)chainBase.width, (int)chainBase.height },
				.mipLevels = (UINT16)mipChain.GetImageCount(),
				.format = chainBase.format,
				.pDevice = pDevice_->GetD3D12DeviceInterface().Get(),
			};
			auto pTexture = textureFactory_(texParams);
			// create the intermediate upload buffer (staging texture, freed automatically on return)
			ComPtr<ID3D12Resource> pIntermediate;
			{
				const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
				const auto uploadBufferSize = GetRequiredIntermediateSize(
					&pTexture->GetResource(), 0, (UINT)subresourceData.size()
				);
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
				texParams.pDevice->CreateCommittedResource(
					&heapProps,
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&pIntermediate)
				) >> chk;
			}
			// write commands to copy data to upload texture (copying each subresource)
			auto cmd = pQueue_->GetCommandListPair();
			UpdateSubresources(
				cmd.pCommandList.Get(),
				&pTexture->GetResource(),
				pIntermediate.Get(),
				0, 0,
				(UINT)subresourceData.size(),
				subresourceData.data()
			); // TODO: check return for error? (0 == error)
			// write command to transition texture to texture state 
			{
				const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					&pTexture->GetResource(),
					D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				cmd.pCommandList->ResourceBarrier(1, &barrier);
			}
			// queue up gpu copy commands
			const auto signal = pQueue_->ExecuteCommandListWithFence(std::move(cmd));
			// wait for copy to complete
			pQueue_->WaitForFenceValue(signal);

			return std::static_pointer_cast<gfx::ITexture>(pTexture);
		});
	}
}