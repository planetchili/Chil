#include "CliFramework.h"


namespace chil::cli
{
	std::optional<int> OptionsContainerBase_::Init_() noexcept
	{
		try {
			app_.parse(__argc, __argv);
			finalized_ = true;
			return {};
		}
		catch (const CLI::ParseError& e) {
			return app_.exit(e);
		}
	}


	OptionsElementBase_::operator bool() const
	{
		return (bool)*pOption_;
	}
	bool OptionsElementBase_::operator!() const
	{
		return !bool(*this);
	}
	std::string OptionsElementBase_::GetName() const
	{
		return pOption_->get_name();
	}
	CLI::App& OptionsElementBase_::GetApp_(OptionsContainerBase_* pContainer)
	{
		return pContainer->app_;
	}
}