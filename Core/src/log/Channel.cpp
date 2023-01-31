#pragma once
#include "Channel.h"
#include "Driver.h"
#include "Policy.h"

namespace chil::log
{
	Channel::Channel(std::vector<std::shared_ptr<IDriver>> driverPtrs)
		:
		driverPtrs_{ std::move(driverPtrs) }
	{}
	Channel::~Channel()
	{}
	void Channel::Submit(Entry& e)
	{
		for (auto& pPolicy : policyPtrs_) {
			if (!pPolicy->TransformFilter(e)) {
				return;
			}
		}
		for (auto& pDriver : driverPtrs_) {
			pDriver->Submit(e);
		}
		// TODO: log case when there are no drivers?
	}
	void Channel::AttachDriver(std::shared_ptr<IDriver> pDriver)
	{
		driverPtrs_.push_back(std::move(pDriver));
	}
	void Channel::AttachPolicy(std::unique_ptr<IPolicy> pPolicy)
	{
		policyPtrs_.push_back(std::move(pPolicy));
	}
}