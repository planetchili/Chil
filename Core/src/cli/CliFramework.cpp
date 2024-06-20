#include "CliFramework.h"


namespace chil::cli
{
	std::optional<int> OptionsContainerBase_::Init_(bool captureDiagnostics) noexcept
	{
		try {
			app_.parse(__argc, __argv);
			finalized_ = true;
			return {};
		}
		catch (const CLI::ParseError& e) {
			if (captureDiagnostics) {
				return app_.exit(e, diagnostics_, diagnostics_);
			}
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


	Flag::Flag(OptionsContainerBase_* pParent, std::string names, std::string description)
	{
		pOption_ = GetApp_(pParent).add_flag(std::move(names), data_, std::move(description));
	}
}