#pragma once
#include <optional>
#include <deque>

namespace chil::gfx
{
	template<class T>
	class FrameResourcePool
	{
	public:
		std::optional<T> GetResource(uint64_t frameFenceValue)
		{
			std::optional<T> resource;
			if (!resourceEntryQueue_.empty() &&
				resourceEntryQueue_.front().frameFenceValue <= frameFenceValue) {
				resource = std::move(resourceEntryQueue_.front().pResource);
				resourceEntryQueue_.pop_front();
			}
			return resource;
		}
		void PutResource(T resource, uint64_t frameFenceValue)
		{
			resourceEntryQueue_.push_back(ResourceEntry_{ frameFenceValue, std::move(resource) });
		}
	private:
		struct ResourceEntry_
		{
			uint64_t frameFenceValue;
			T pResource;
		};
		std::deque<ResourceEntry_> resourceEntryQueue_;
	};
}