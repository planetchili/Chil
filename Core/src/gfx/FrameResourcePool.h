#pragma once
#include <optional>
#include <deque>
#include <ranges>

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
		void Clear()
		{
			garbagePile_.append_range(resourceEntryQueue_ | std::views::as_rvalue);
			resourceEntryQueue_.clear();
		}
		void CollectGarbage(uint64_t fenceValue)
		{
			std::erase_if(garbagePile_, [=](const ResourceEntry_& re) { return re.frameFenceValue <= fenceValue; });
		}
	private:
		struct ResourceEntry_
		{
			uint64_t frameFenceValue;
			T pResource;
		};
		std::deque<ResourceEntry_> resourceEntryQueue_;
		std::vector<ResourceEntry_> garbagePile_;
	};
}