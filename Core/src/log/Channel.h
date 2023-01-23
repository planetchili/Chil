#pragma once

namespace chil::log
{
	struct Entry;

	class IChannel
	{
	public:
		virtual ~IChannel() = default;
		virtual void Submit(Entry&) = 0;
	};
}