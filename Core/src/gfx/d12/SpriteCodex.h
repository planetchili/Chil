#pragma once
#include "Device.h"
#include "Texture.h"
#include "../ISpriteCodex.h"
#include <memory>
#include <vector>



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
		SpriteCodex(std::shared_ptr<IDevice> pDevice, UINT initialCapacity = 4);
		std::shared_ptr<Atlas> AddAtlas(std::shared_ptr<gfx::ITexture> pTexture) override;
		std::vector<std::shared_ptr<Atlas>> GetAtlases() const override;
		ID3D12DescriptorHeap* GetHeap() const override;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTableHandle() const override;
	private:
		// types
		struct Atlas_ : public Atlas
		{
			Atlas_(uint32_t index, std::shared_ptr<ITexture> pTexture);
			spa::DimensionsI GetDimensions() const override;
			D3D12_CPU_DESCRIPTOR_HANDLE srvHandle_;
			std::shared_ptr<ITexture> pTexture_;
		};
		// functions
		void ResizeHeap_(UINT newSize);
		void BindAtlasToDescriptorHeap_(Atlas_& atlas, UINT index);
		// data
		std::shared_ptr<IDevice> pDevice_;
		UINT descriptorSize_;
		UINT heapCapacity_;
		UINT nextHeapIndex_ = 0;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pSrvHeap_;
		std::vector<std::shared_ptr<Atlas_>> spriteAtlases_;
	};
}