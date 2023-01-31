#pragma once
#include <string>

namespace chil::log
{
	struct Entry;

	class ITextFormatter
	{
	public:
		virtual ~ITextFormatter() = default;
		virtual std::wstring Format(const Entry&) const = 0;
	};

	class TextFormatter : public ITextFormatter
	{
	public:
		std::wstring Format(const Entry&) const override;
	};
}