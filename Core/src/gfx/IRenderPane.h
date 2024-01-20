#pragma once
#include <Core/src/win/ChilWin.h>
#include <Core/src/spa/Dimensions.h>

namespace chil::gfx
{
	class IRenderPane
	{
	public:
		// types
		struct IocParams
		{
			HWND hWnd;
			spa::DimensionsI dims;
		};
		// functions
		virtual ~IRenderPane() = default;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void FlushQueues() const = 0;
	};
}