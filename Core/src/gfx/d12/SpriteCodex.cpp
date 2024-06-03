#include "SpriteCodex.h"
#include <Core/src/utl/Assert.h>
#include <Core/src/utl/HrChecker.h>
#include "../ISpriteBatcher.h"


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
				.NumDescriptors = (UINT)maxNumAtlases + 1, // +1 to account for the cbv
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			};
			pDevice_->GetD3D12DeviceInterface()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&pHeap_)) >> chk;
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
		auto descriptorHandle = pHeap_->GetCPUDescriptorHandleForHeapStart();
		descriptorHandle.ptr += SIZE_T(descriptorSize_) * (SIZE_T(curNumAtlases_) + 1); // +1 to account for the cbv
		// write into descriptor
		pTextureD12->WriteDescriptor(pDevice_->GetD3D12DeviceInterface().Get(), descriptorHandle);
		// store in atlas array
		spriteAtlases_.push_back(std::make_unique<SpriteAtlas_>(descriptorHandle, std::move(pTextureD12)));
		// update number of atlases stored
		curNumAtlases_++;
	}

	ID3D12DescriptorHeap* SpriteCodex::GetHeap() const
	{
		return pHeap_.Get();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE SpriteCodex::GetTableHandle() const
	{
		return pHeap_->GetGPUDescriptorHandleForHeapStart();
	}

	spa::DimensionsI SpriteCodex::GetAtlasDimensions(size_t atlasIndex) const
	{
		chilass(atlasIndex < curNumAtlases_);

		return spriteAtlases_[atlasIndex]->pTexture_->GetDimensions();
	}
	std::shared_ptr<ISpriteFrame> SpriteCodex::AddFrame(const spa::RectF& frameInPixels, size_t atlasIndex)
	{
		throw std::runtime_error{""};
	}
	std::shared_ptr<ISpriteFrame> SpriteCodex::AddFrame(const spa::DimensionsI& cellDimension, const spa::Vec2I& cellCoordinates, size_t atlasIndex)
	{
		auto pFrame = std::make_shared<SpriteFrame>((uint16_t)spriteFrames_.size(),
			cellDimension, cellCoordinates, atlasIndex, shared_from_this());
		spriteFrames_.push_back(pFrame);
		return pFrame;
	}

	bool SpriteCodex::NeedsFinalization() const
	{
		return !pStructuredBuffer_;
	}

	void SpriteCodex::Finalize(CommandListPair& clp, uint64_t frameFenceValue)
	{
		// write data to gpu
		std::vector<SpriteFrame::Data> data;
		data.reserve(spriteFrames_.size());
		for (auto f : spriteFrames_) {
			data.push_back(f->PoopData());
		}
		pStructuredBuffer_ = std::make_unique<StructuredBuffer>(*pDevice_, data);
		pStructuredBuffer_->WriteCopyCommands(clp, frameFenceValue);
		// write descriptor
		pStructuredBuffer_->WriteDescriptor(
			pDevice_->GetD3D12DeviceInterface().Get(),
			pHeap_->GetCPUDescriptorHandleForHeapStart()
		);
	}

	SpriteFrame::SpriteFrame(uint16_t frameIndex, const spa::RectF& frameInPixels, size_t atlasIndex, std::shared_ptr<ISpriteCodex> pCodex)
		:
		atlasIndex_{ (uint16_t)atlasIndex },
		pCodex_{ std::move(pCodex) },
		frameIndex_{ frameIndex }
	{
		atlasDimensions_ = pCodex_.lock()->GetAtlasDimensions(atlasIndex_);
		frameInTexcoords_ = {
			.left = frameInPixels.left / atlasDimensions_.width,
			.top = frameInPixels.top / atlasDimensions_.height,
			.right = frameInPixels.right / atlasDimensions_.width,
			.bottom = frameInPixels.bottom / atlasDimensions_.height,
		};
		pivotInPixelCoordinates_ = {
			0.f,
			-(frameInPixels.bottom - frameInPixels.top) / 2.f,
		};
	}

	SpriteFrame::SpriteFrame(uint16_t frameIndex, const spa::DimensionsI& cellGridDimensions, const spa::Vec2I& cellCoordinates, size_t atlasIndex, std::shared_ptr<ISpriteCodex> pCodex)
		:
		atlasIndex_{ (uint16_t)atlasIndex },
		pCodex_{ std::move(pCodex) },
		frameIndex_{ frameIndex }
	{
		atlasDimensions_ = pCodex_.lock()->GetAtlasDimensions(atlasIndex_);
		const auto cellWidth = atlasDimensions_.width / float(cellGridDimensions.width);
		const auto cellHeight = atlasDimensions_.height / float(cellGridDimensions.height);
		const auto frameInPixels = spa::RectF{
			.left = cellWidth * cellCoordinates.x,
			.top = cellHeight * cellCoordinates.y,
			.right = cellWidth * (cellCoordinates.x + 1),
			.bottom = cellHeight * (cellCoordinates.y + 1),
		};
		frameInTexcoords_ = {
			.left = frameInPixels.left / atlasDimensions_.width,
			.top = frameInPixels.top / atlasDimensions_.height,
			.right = frameInPixels.right / atlasDimensions_.width,
			.bottom = frameInPixels.bottom / atlasDimensions_.height,
		};
		pivotInPixelCoordinates_ = {
			0.f,
			-(frameInPixels.bottom - frameInPixels.top) / 2.f,
		};
	}

	SpriteFrame::Data SpriteFrame::PoopData() const
	{
		return Data{
			.pivotPixelCoords = pivotInPixelCoordinates_,
			.frameTexPos = frameInTexcoords_.GetTopLeft(),
			.frameTexDims = frameInTexcoords_.GetDimensions(),
			.destPixelDims = frameInTexcoords_.GetDimensions() * atlasDimensions_,
			.atlasIndex = (uint16_t)atlasIndex_,
		};
	}

	void SpriteFrame::DrawToBatch(ISpriteBatcher& batch, const spa::Vec2F& pos, float rotation, float zOrder, const spa::DimensionsF& scale) const
	{
		batch.Draw(frameIndex_, pos, rotation, zOrder, scale);
	}
}