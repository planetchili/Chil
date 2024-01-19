#include "ResourceLoader.h"
#include "CommandQueue.h"
#include "Texture.h"

namespace chil::gfx::d12
{
	ResourceLoader::ResourceLoader(std::shared_ptr<gfx::IDevice> pDevice)
		:
		pDevice_{ std::dynamic_pointer_cast<decltype(pDevice_)::element_type>(std::move(pDevice)) },
		// TODO: inject this to enable IoC
		pQueue_{ std::make_shared<CommandQueue>(pDevice_) }
	{}

	std::future<std::shared_ptr<gfx::ITexture>> ResourceLoader::LoadTexture(std::wstring path)
	{
		// - consider batching multiple loads together in a single command list
		// - consider reusing intermediates / allocating from heap instead of committed
		// - consider using copy / compute queue (still need to transition resource in a graphics queue before use)
		return std::async([=]() mutable {
			auto cmd = pQueue_->GetCommandListPair();
			// TODO: use IoC to get the texture somehow
			// idea: inject a factory into loader, which could be IoC or IoC wrapper, or could be something else
			auto pTexture = std::make_shared<Texture>(pDevice_->GetD3D12DeviceInterface(), cmd, std::move(path));
			const auto signal = pQueue_->ExecuteCommandListWithFence(std::move(cmd));
			pQueue_->WaitForFenceValue(signal);
			pTexture->ClearIntermediate();
			return std::static_pointer_cast<gfx::ITexture>(pTexture);
		});
	}
}