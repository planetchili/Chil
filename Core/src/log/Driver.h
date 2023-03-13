#pragma once
#include <memory>

namespace chil::log
{
	struct Entry;
	class ITextFormatter;

	class IDriver
	{
	public:
		virtual ~IDriver() = default;
		virtual void Submit(const Entry&) = 0;
		virtual void Flush() = 0;
	};

	class ITextDriver : public IDriver
	{
	public:
		virtual void SetFormatter(std::shared_ptr<ITextFormatter>) = 0;
	};
}