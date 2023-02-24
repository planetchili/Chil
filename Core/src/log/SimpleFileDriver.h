#pragma once 
#include "Driver.h" 
#include <memory> 
#include <filesystem> 
#include <fstream> 

namespace chil::log
{
	class ISimpleFileDriver : public ITextDriver {};

	class SimpleFileDriver : public ISimpleFileDriver
	{
	public:
		SimpleFileDriver(std::filesystem::path path, std::shared_ptr<ITextFormatter> pFormatter = {});
		void Submit(const Entry&) override;
		void SetFormatter(std::shared_ptr<ITextFormatter> pFormatter) override;
	private:
		std::wofstream file_;
		std::shared_ptr<ITextFormatter> pFormatter_;
	};
}