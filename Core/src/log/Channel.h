#pragma once

namespace chil::log
{
	struct Entry;

	class IChannel
	{
	public:
		virtual void Submit(Entry&) = 0;
	};
}