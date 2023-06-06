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
		auto cmd = pQueue_->GetCommandListPair();
		// TODO:	move CPU code into lauch async
		//			move loading logic from Texture to Loader, Texture just holds the results
		auto pTexture = std::make_shared<Texture>(pDevice_->GetD3D12DeviceInterface(), cmd, std::move(path));
		const auto signal = pQueue_->ExecuteCommandList(std::move(cmd));
		return std::async(std::launch::deferred, [=] {
			pQueue_->WaitForFenceValue(signal);
			pTexture->ClearIntermediate();
			return std::static_pointer_cast<ITexture>(pTexture);
		});
	}
}