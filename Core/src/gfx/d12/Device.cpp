#include "Device.h"
#include <Core/src/utl/HrChecker.h>
#include <Core/src/log/Log.h>
#pragma warning(push)
#pragma warning(disable : 26495)
#include "d3dx12.h"
#pragma warning(pop)

namespace chil::gfx::d12
{
	using utl::chk;
	using Microsoft::WRL::ComPtr;

	Device::Device()
	{
		// make an env namespace / header for build switch stuff like this
#ifdef NDEBUG
		constexpr bool is_debug = false;
#else
		constexpr bool is_debug = true;
#endif
		// enable the software debug layer for d3d12
		if (is_debug) {
			ComPtr<ID3D12Debug1> debugController;
			D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)) >> chk;
			debugController->EnableDebugLayer();
			debugController->SetEnableGPUBasedValidation(true);
		}
		CreateDXGIFactory2(is_debug ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&pDxgiFactory_)) >> chk;
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