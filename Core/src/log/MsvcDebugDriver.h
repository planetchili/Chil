#pragma once
#include "Driver.h"
#include <memory>

namespace chil::log
{
	class IMsvcDebugDriver : public ITextDriver {};

	class MsvcDebugDriver : public IMsvcDebugDriver
	{
	public:
		MsvcDebugDriver(std::shared_ptr<ITextFormatter> pFormatter = {});
		void Submit(const Entry&) override;
		void SetFormatter(std::shared_ptr<ITextFormatter> pFormatter) override;
		void Flush() override;
	private:
		std::shared_ptr<ITextFormatter> pFormatter_;
	};
}