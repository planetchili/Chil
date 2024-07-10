#include "CliFramework.h"
#include "WrapCli11.h"
#include <magic_enum.hpp>
#include <Core/src/crn/Ranges.h>

namespace rn = std::ranges;
namespace vi = rn::views;

namespace chil::cli
{
	template<typename T>
	T& OptionsContainer<T>::Get_()
	{
		static T opts;
		return opts;
	}

	template<typename T>
	Option<T>::Option(OptionsContainerBase_* pParent, std::string names, std::string description, std::optional<T> def, const cust::Customizer& customizer)
		:
		data_{ def ? std::move(*def) : T{} }
	{
		pOption_ = GetApp_(pParent).add_option(std::move(names), data_, std::move(description));
		if (def) {
			pOption_->default_val(data_);
		}
		customizer.Invoke(pOption_);
	}

	namespace rule
	{
		template<class T, class...Rest>
		void MutualExclusion::ExcludeRecursive_(T& pivot, Rest&...rest)
		{
			if constexpr (sizeof...(rest) > 0) {
				GetOption_(pivot)->excludes(GetOption_(rest)...);
				ExcludeRecursive_(rest...);
			}
		}

		template<class Pivot, class Dependent>
		void Dependency::DependencyImpl_(Pivot& pivot, Dependent& dependent)
		{
			GetOption_(pivot)->needs(GetOption_(dependent));
		}
	}

	namespace cust
	{
		namespace
		{
			Customizer Adapt_(CLI::Validator v)
			{
				return Customizer([=](CLI::Option* pOpt) {
					pOpt->transform(v);
				});
			}
			template<class V, typename...T>
			Customizer Adapt_(T&&...args)
			{
				return Adapt_(V{ args... });
			}
			std::string NullModifier_(std::string) { return {}; }
		}

		Customizer ExistingPath() { return Adapt_(CLI::ExistingPath); }
		Customizer ExistingDirectory() { return Adapt_(CLI::ExistingDirectory); }
		Customizer ExistingFile() { return Adapt_(CLI::ExistingFile); }
		Customizer NonExistingPath() { return Adapt_(CLI::NonexistentPath); }
		Customizer Ipv4() { return Adapt_(CLI::ValidIPV4); }
		Customizer NonNegative() { return Adapt_(CLI::NonNegativeNumber); }
		Customizer Positive() { return Adapt_(CLI::PositiveNumber); }

		template<typename T>
		Customizer Range(T min, T max) { return Adapt_<CLI::Range>(min, max); }

		template<typename T>
		Customizer Clamp(T min, T max) { return Adapt_<CLI::Bound>(min, max); }

		template<bool IgnoreCase, typename T>
		Customizer In(T&& container)
		{
			using ValueType = typename std::decay_t<T>::value_type;
			constexpr bool IgnoreCaseOverride = std::same_as<std::string, ValueType> || std::same_as<const char*, ValueType> ?
				IgnoreCase : false;
			if constexpr (IgnoreCaseOverride) {
				return Adapt_<CLI::IsMember>(container, CLI::ignore_case);
			}
			else {
				return Adapt_<CLI::IsMember>(container);
			}
		}

		template<bool IgnoreCase, typename T>
		Customizer In(std::initializer_list<T> values)
		{
			return In<IgnoreCase>(std::vector<T>(values));
		}

		template<typename E>
		Customizer EnumMap(bool ignoreCase)
		{
			using namespace magic_enum;
			auto map = vi::zip(enum_names<E>() | crn::Cast<std::string>(), enum_values<E>()) | rn::to<std::map>();
			return Adapt_<CLI::CheckedTransformer>(std::move(map), ignoreCase ? CLI::ignore_case : NullModifier_);
		}
	}
}