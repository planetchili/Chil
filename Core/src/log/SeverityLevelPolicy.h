#pragma once
#include "Policy.h"
#include "Level.h"

namespace chil::log
{
	class ISeverityLevelPolicy : public IPolicy {};

	class SeverityLevelPolicy : public ISeverityLevelPolicy
	{
	public:
		SeverityLevelPolicy(Level level);
		bool TransformFilter(Entry&) override;
	private:
		Level level_;
	};
}