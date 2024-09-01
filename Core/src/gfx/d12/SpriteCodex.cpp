#include "SpriteCodex.h"
#include <Core/src/utl/Assert.h>
#include <Core/src/utl/HrChecker.h>
#include <Core/src/crn/Ranges.h>


namespace chil::gfx::d12
{
	using utl::chk;
	namespace rn = std::ranges;

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

	std::shared_ptr<gfx::ISpriteCodex::Atlas> SpriteCodex::AddAtlas(std::shared_ptr<gfx::ITexture> pTexture)
	{
		chilass(spriteAtlases_.size() < maxNumAtlases_);

		// downcast to platform-specific interface
		auto pTextureD12 = std::dynamic_pointer_cast<d12::ITexture>(std::move(pTexture));

		// get handle to the destination descriptor
		auto descriptorHandle = pSrvHeap_->GetCPUDescriptorHandleForHeapStart();
		descriptorHandle.ptr += SIZE_T(descriptorSize_) * SIZE_T(spriteAtlases_.size());
		// write into descriptor
		pTextureD12->WriteDescriptor(pDevice_->GetD3D12DeviceInterface().Get(), descriptorHandle);
		// store in atlas array
		spriteAtlases_.push_back(std::make_shared<Atlas_>(uint32_t(spriteAtlases_.size()),
			descriptorHandle, std::move(pTextureD12)));
		// return sptr to newly-added atlas
		return spriteAtlases_.back();
	}

	ID3D12DescriptorHeap* SpriteCodex::GetHeap() const
	{
		return pSrvHeap_.Get();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE SpriteCodex::GetTableHandle() const
	{
		return pSrvHeap_->GetGPUDescriptorHandleForHeapStart();
	}

	std::vector<std::shared_ptr<SpriteCodex::Atlas>> SpriteCodex::GetAtlases() const
	{
		return spriteAtlases_ | crn::Cast<std::shared_ptr<SpriteCodex::Atlas>>() | rn::to<std::vector>();
	}

	SpriteCodex::Atlas_::Atlas_(uint32_t index, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, std::shared_ptr<ITexture> pTexture)
		:
		Atlas{ index },
		srvHandle_{ srvHandle },
		pTexture_{ pTexture }
	{}

	spa::DimensionsI SpriteCodex::Atlas_::GetDimensions() const
	{
		return pTexture_->GetDimensions();
	}
}