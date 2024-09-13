#include "SpriteCodex.h"
#include <Core/src/utl/Assert.h>
#include <Core/src/utl/HrChecker.h>
#include <Core/src/crn/Ranges.h>


namespace chil::gfx::d12
{
	using utl::chk;
	namespace rn = std::ranges;
	namespace vi = std::views;

	SpriteCodex::SpriteCodex(std::shared_ptr<IDevice> pDevice, UINT initialCapacity)
		:
		pDevice_{ std::move(pDevice) }
	{
		// size of descriptors used for index calculation
		descriptorSize_ = pDevice_->GetD3D12DeviceInterface()->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		ResizeHeap_(initialCapacity);
	}

	std::shared_ptr<gfx::ISpriteCodex::Atlas> SpriteCodex::AddAtlas(std::shared_ptr<gfx::ITexture> pTexture)
	{
		// resize heap if necessary (2^n growth)
		if (nextHeapIndex_ >= heapCapacity_) {
			// resize by doubling, start at 4 if currently zero capacity
			ResizeHeap_(heapCapacity_ ? heapCapacity_ * 2 : 4);
		}
		// downcast texture to platform-specific interface
		auto pTextureD12 = std::dynamic_pointer_cast<d12::ITexture>(std::move(pTexture));
		// create the atlas
		auto pAtlas = std::make_shared<Atlas_>(uint32_t(spriteAtlases_.size()), std::move(pTextureD12));
		// bind atlas to the next available heap slot
		BindAtlasToDescriptorHeap_(*pAtlas, nextHeapIndex_++);
		// store in atlas array
		spriteAtlases_.push_back(pAtlas);
		// return sptr to newly-added atlas
		return pAtlas;
	}

	ID3D12DescriptorHeap* SpriteCodex::GetHeap() const
	{
		return pSrvHeap_.Get();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE SpriteCodex::GetTableHandle() const
	{
		return pSrvHeap_->GetGPUDescriptorHandleForHeapStart();
	}

	void SpriteCodex::ResizeHeap_(UINT newSize)
	{
		chilass(newSize >= (UINT)spriteAtlases_.size());
		heapCapacity_ = newSize;
		// atlases will be defragmented and next index comes at the end
		nextHeapIndex_ = (UINT)spriteAtlases_.size();
		// if size is zero, free the heap and return
		if (newSize == 0) {
			pSrvHeap_.Reset();
			return;
		}
		// allocate descriptor heap for srvs of the atlases (textures)
		{
			const D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = (UINT)heapCapacity_,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			};
			pDevice_->GetD3D12DeviceInterface()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&pSrvHeap_)) >> chk;
		}
		// emplace existing atlases into the heap
		for (auto&&[i, pAtlas] : vi::enumerate(spriteAtlases_)) {
			BindAtlasToDescriptorHeap_(*pAtlas, (UINT)i);
		}
	}

	void SpriteCodex::BindAtlasToDescriptorHeap_(Atlas_& atlas, UINT index)
	{
		chilass(index < heapCapacity_);
		// get handle to the destination descriptor
		auto descriptorHandle = pSrvHeap_->GetCPUDescriptorHandleForHeapStart();
		descriptorHandle.ptr += SIZE_T(descriptorSize_) * SIZE_T(index);
		// write into descriptor
		atlas.pTexture_->WriteDescriptor(pDevice_->GetD3D12DeviceInterface().Get(), descriptorHandle);
		// set index on atlas
		atlas.index = index;
	}

	std::vector<std::shared_ptr<SpriteCodex::Atlas>> SpriteCodex::GetAtlases() const
	{
		return spriteAtlases_ | crn::Cast<std::shared_ptr<SpriteCodex::Atlas>>() | rn::to<std::vector>();
	}

	SpriteCodex::Atlas_::Atlas_(uint32_t index, std::shared_ptr<ITexture> pTexture)
		:
		Atlas{ index },
		srvHandle_{},
		pTexture_{ pTexture }
	{}

	spa::DimensionsI SpriteCodex::Atlas_::GetDimensions() const
	{
		return pTexture_->GetDimensions();
	}
}