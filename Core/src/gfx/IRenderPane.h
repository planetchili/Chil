#pragma once

namespace chil::gfx
{
	class IRenderPane
	{
	public:
		virtual ~IRenderPane() = default;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void FlushQueues() const = 0;
	};
}