#include "ChilCppUnitTest.h"
#include <Core/src/win/Window.h>
#include <Core/src/win/WindowClass.h>
#include <Core/src/gfx/d12/Device.h>
#include <Core/src/gfx/d12/RenderPane.h>


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
			win::Window window{ std::make_shared<win::WindowClass>(), L"Gfx Device Test Window", { 800, 600 } };
			auto pDevice = std::make_shared<gfx::d12::Device>();
			gfx::d12::RenderPane pane{ window.GetHandle(), { 800, 600 }, pDevice };
			Assert::IsTrue(true);
		}
	};
}
