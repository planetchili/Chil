#include "ResourceLoader.h"
#include "CommandQueue.h"

namespace chil::gfx::d12
{
	ResourceLoader::ResourceLoader(std::shared_ptr<IDevice> pDevice)
		:
		pDevice_{ std::move(pDevice) },
		pQueue_{ std::make_shared<CommandQueue>(pDevice_) }
	{}

	std::future<std::shared_ptr<ITexture>> ResourceLoader::LoadTexture(std::wstring path)
	{
		// - consider batching multiple loads together in a single command list
		// - consider reusing intermediates / allocating from heap instead of committed
		// - consider using copy / compute queue (still need to transition resource in a graphics queue before use)
		return std::async([=]() mutable {
			auto cmd = pQueue_->GetCommandListPair();
			auto pTexture = std::make_shared<Texture>(pDevice_->GetD3D12DeviceInterface(), cmd, std::move(path));
			const auto signal = pQueue_->ExecuteCommandListWithFence(std::move(cmd));
			pQueue_->WaitForFenceValue(signal);
			pTexture->ClearIntermediate();
			return std::static_pointer_cast<ITexture>(pTexture);
		});
	}
}