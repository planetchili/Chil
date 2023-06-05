#include "Device.h"
#include <Core/src/utl/HrChecker.h>
#include <Core/src/log/Log.h>
#pragma warning(push)
#pragma warning(disable : 26495)
#include "d3dx12.h"
#pragma warning(pop)
#include "Texture.h"

namespace chil::gfx::d12
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;

	Device::Device()
	{
		// enable the software debug layer for d3d12 
		{
			ComPtr<ID3D12Debug1> debugController;
			D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)) >> chk;
			debugController->EnableDebugLayer();
			debugController->SetEnableGPUBasedValidation(true);
		}
		CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&pDxgiFactory_)) >> chk;
		D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&pDevice_)) >> chk;
	}

	Device::~Device() = default;

	ComPtr<ID3D12Device2> Device::GetD3D12DeviceInterface()
	{
		return pDevice_;
	}

	ComPtr<IDXGIFactory4> Device::GetDXGIFactoryInterface()
	{
		return pDxgiFactory_;
	}

	std::future<std::shared_ptr<ITexture>> Device::LoadTexture(std::wstring path)
	{
		auto cmd = pResourceQueue_->GetCommandListPair();
		auto pTexture = std::make_shared<Texture>(pDevice_, cmd, std::move(path));
		const auto signal = pResourceQueue_->ExecuteCommandList(std::move(cmd));
		return std::async(std::launch::deferred, [=] {
			pResourceQueue_->WaitForFenceValue(signal);
			return std::static_pointer_cast<ITexture>(pTexture);
		});
	}
}