#pragma once
#include "Device.h"
#include "Texture.h"
#include "../ISpriteCodex.h"
#include <memory>


namespace chil::gfx::d12
{
	class ISpriteCodex : public gfx::ISpriteCodex
	{
	public:
		virtual ID3D12DescriptorHeap* GetHeap() const = 0;
		virtual D3D12_GPU_DESCRIPTOR_HANDLE GetTableHandle() const = 0;
	};

	class SpriteCodex : public ISpriteCodex
	{
	public:
		SpriteCodex(std::shared_ptr<IDevice> pDevice, UINT maxNumAtlases = 4);
		void AddSpriteAtlas(std::shared_ptr<gfx::ITexture> pTexture) override;
		ID3D12DescriptorHeap* GetHeap() const override;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTableHandle() const override;
		spa::DimensionsI GetAtlasDimensions(size_t atlasIndex) const override;
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