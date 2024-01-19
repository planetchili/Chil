#include "SpriteCodex.h"
#include <Core/src/utl/Assert.h>
#include <Core/src/utl/HrChecker.h>


namespace chil::gfx::d12
{
	using utl::chk;

	SpriteCodex::SpriteCodex(std::shared_ptr<IDevice> pDevice, UINT maxNumAtlases)
		:
		pDevice_{ std::move(pDevice) },
		maxNumAtlases_{ maxNumAtlases }
	{
		// descriptor heap for srvs
		{
			const D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = (UINT)maxNumAtlases,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			};
			pDevice_->GetD3D12DeviceInterface()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&pSrvHeap_)) >> chk;
		}
		// size of descriptors used for index calculation
		descriptorSize_ = pDevice_->GetD3D12DeviceInterface()->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void SpriteCodex::AddSpriteAtlas(std::shared_ptr<gfx::ITexture> pTexture)
	{
		chilass(curNumAtlases_ < maxNumAtlases_);

		// downcast to platform-specific interface
		auto pTextureD12 = std::dynamic_pointer_cast<d12::ITexture>(std::move(pTexture));

		// get handle to the destination descriptor
		auto descriptorHandle = pSrvHeap_->GetCPUDescriptorHandleForHeapStart();
		descriptorHandle.ptr += SIZE_T(descriptorSize_) * SIZE_T(curNumAtlases_);
		// write into descriptor
		pTextureD12->WriteDescriptor(pDevice_->GetD3D12DeviceInterface().Get(), descriptorHandle);
		// store in atlas array
		spriteAtlases_.push_back(std::make_unique<SpriteAtlas_>(descriptorHandle, std::move(pTextureD12)));
		// update number of atlases stored
		curNumAtlases_++;
	}

	ID3D12DescriptorHeap* SpriteCodex::GetHeap() const
	{
		return pSrvHeap_.Get();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE SpriteCodex::GetTableHandle() const
	{
		return pSrvHeap_->GetGPUDescriptorHandleForHeapStart();
	}

	spa::DimensionsI SpriteCodex::GetAtlasDimensions(size_t atlasIndex) const
	{
		chilass(atlasIndex < curNumAtlases_);

		return spriteAtlases_[atlasIndex]->pTexture_->GetDimensions();
	}
}