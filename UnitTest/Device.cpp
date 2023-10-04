#include "ChilCppUnitTest.h"
#include <Core/src/win/Window.h>
#include <Core/src/win/WindowClass.h>
#include <Core/src/gfx/d12/Device.h>
#include <Core/src/gfx/d12/RenderPane.h>
#include <Core/src/gfx/d12/CommandQueue.h>
#include <Core/src/win/Input.h>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace chil;
using namespace std::string_literals;

namespace Gfx
{
	TEST_CLASS(GfxD12)
	{
	public:
		// testing text formatting
		TEST_METHOD(TestDevicePaneConstruction)
		{
			auto input = std::make_shared<win::Keyboard>();
			win::Window window{ std::make_shared<win::WindowClass>(), input, L"Gfx Device Test Window", { 800, 600 } };
			auto pDevice = std::make_shared<gfx::d12::Device>();
			auto pCommandQueue = std::make_shared<gfx::d12::CommandQueue>(pDevice);
			gfx::d12::RenderPane pane{ window.GetHandle(), { 800, 600 }, pDevice, std::move(pCommandQueue) };
			Assert::IsTrue(true);
		}
	};
}
