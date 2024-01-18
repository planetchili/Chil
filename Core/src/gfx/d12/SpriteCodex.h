#pragma once
#include "Device.h"
#include "Texture.h"
#include <memory>


namespace chil::gfx::d12
{
	class SpriteCodex
	{
	public:
		SpriteCodex(std::shared_ptr<IDevice> pDevice, UINT maxNumAtlases = 4);
		void AddSpriteAtlas(std::shared_ptr<ITexture> pTexture);
		ID3D12DescriptorHeap* GetHeap() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTableHandle() const;
		spa::DimensionsI GetAtlasDimensions(size_t atlasIndex) const;
	private:
		// types
		struct SpriteAtlas_
		{
			D3D12_CPU_DESCRIPTOR_HANDLE srvHandle_;
			std::shared_ptr<ITexture> pTexture_;
		};
		// data
		std::shared_ptr<IDevice> pDevice_;
		UINT descriptorSize_;
		UINT maxNumAtlases_;
		UINT curNumAtlases_ = 0;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pSrvHeap_;
		std::vector<std::unique_ptr<SpriteAtlas_>> spriteAtlases_;
	};
}