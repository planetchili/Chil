#include "CliFramework.h"
#include "WrapCli11.h"

namespace chil::cli
{
	template<typename T>
	T& OptionsContainer<T>::Get_()
	{
		static T opts;
		return opts;
	}

	template<typename T>
	template<class C>
	Option<T>::Option(OptionsContainerBase_* pParent, std::string names, std::string description, std::optional<T> def, const C& cust)
		:
		data_{ def ? std::move(*def) : T{} }
	{
		pOption_ = GetApp_(pParent).add_option(std::move(names), data_, std::move(description));
		if (def) {
			pOption_->default_val(data_);
		}
		if constexpr (std::invocable<C, CLI::Option*>) {
			cust(pOption_);
		}
		else if constexpr (std::is_base_of_v<CLI::Validator, C>) {
			pOption_->transform(cust);
		}
	}
}