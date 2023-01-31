#pragma once
#include "Driver.h"
#include <memory>

namespace chil::log
{
	class MsvcDebugDriver : public ITextDriver
	{
	public:
		MsvcDebugDriver(std::unique_ptr<ITextFormatter> pFormatter = {});
		void Submit(const Entry&) override;
		void SetFormatter(std::unique_ptr<ITextFormatter> pFormatter) override;
	private:
		std::unique_ptr<ITextFormatter> pFormatter_;
	};
}