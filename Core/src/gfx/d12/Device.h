#pragma once
#include "../IDevice.h"
#include <Core/src/win/IWindow.h>
#include <d3d12.h> 
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <future>
#include "ICommandQueue.h"
#include "Texture.h"

namespace chil::gfx::d12
{
	class IDevice : public gfx::IDevice
	{
	public:
		virtual Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12DeviceInterface() = 0;
		virtual Microsoft::WRL::ComPtr<IDXGIFactory4> GetDXGIFactoryInterface() = 0;
		virtual std::future<std::shared_ptr<ITexture>> LoadTexture(std::wstring path) = 0;
	};

	class Device : public IDevice
	{
	public:
		Device();
		~Device();
		Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12DeviceInterface() override;
		Microsoft::WRL::ComPtr<IDXGIFactory4> GetDXGIFactoryInterface() override;
		std::future<std::shared_ptr<ITexture>> LoadTexture(std::wstring path) override;
	private:
		Microsoft::WRL::ComPtr<ID3D12Device2> pDevice_;
		Microsoft::WRL::ComPtr<IDXGIFactory4> pDxgiFactory_;
		std::shared_ptr<ICommandQueue> pResourceQueue_;
	};
}