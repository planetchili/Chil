#include "Device.h"
#include <Core/src/utl/HrChecker.h>
#include <Core/src/log/Log.h>

namespace chil::gfx::d11
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;

	Device::Device()
	{
		// enable the software debug layer for d3d12 
		{
			ComPtr<ID3D12Debug> debugController;
			D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)) >> chk;
			debugController->EnableDebugLayer();
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
}