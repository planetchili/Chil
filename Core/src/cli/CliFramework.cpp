#include "CliFramework.h"
#include "WrapCli11.h"
#include <Core/src/utl/String.h>

namespace chil::cli
{
	std::string OptionNameFromElementName(const std::string& ename)
	{
		return "--" + utl::CamelToKebab(ename);
	}

	std::string ComposeFlagName(const std::string& ename, const std::string& shortcut)
	{
		auto fullname = OptionNameFromElementName(ename);
		if (!shortcut.empty()) {
			fullname += ",-" + shortcut;
		}
		return fullname;
	}

	OptionsContainerBase_::OptionsContainerBase_()
		:
		pApp_{ std::make_shared<CLI::App>() }
	{}

	std::optional<int> OptionsContainerBase_::Init_(bool captureDiagnostics) noexcept
	{
		try {
			if (auto name = GetName(); !name.empty()) {
				pApp_->name(std::move(name));
			}
			if (auto desc = GetDesc(); !desc.empty()) {
				pApp_->description(std::move(desc));
			}
			pApp_->parse(__argc, __argv);
			finalized_ = true;
			return {};
		}
		catch (const CLI::ParseError& e) {
			if (captureDiagnostics) {
				return pApp_->exit(e, diagnostics_, diagnostics_);
			}
			return pApp_->exit(e);
		}
	}

	CLI::Option* OptionsContainerBase_::GetOpt_(OptionsElementBase_& el)
	{
		return el.pOption_;
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
		return *pContainer->pApp_;
	}


	Flag::Flag(OptionsContainerBase_* pParent, std::string names, std::string description, const cust::Customizer& customizer)
	{
		pOption_ = GetApp_(pParent).add_flag(std::move(names), data_, std::move(description));
		customizer.Invoke(pOption_);
	}


	namespace rule
	{
		CLI::Option* chil::cli::rule::RuleBase_::GetOption_(OptionsElementBase_& element)
		{
			return element.pOption_;
		}
		CLI::App& RuleBase_::GetApp_(OptionsContainerBase_* pParent)
		{
			return OptionsElementBase_::GetApp_(pParent);
		}


		AllowExtras::AllowExtras(OptionsContainerBase_* pParent)
		{
			GetApp_(pParent).allow_extras(true);
		}
	}
}
