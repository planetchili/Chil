#pragma once
#include "WrapCli11.h"
#include <optional>
#include <Core/src/utl/Assert.h>
#include <sstream>
#include <Core/src/crn/Ranges.h>
#include <magic_enum.hpp>


namespace chil::cli
{
	namespace rn = std::ranges;
	namespace vi = rn::views;

	std::string OptionNameFromElementName(const std::string& ename);
	std::string ComposeFlagName(const std::string& ename, const std::string& shortcut);

	namespace rule
	{
		struct RuleBase_;
	}

	class OptionsContainerBase_
	{
		friend class OptionsElementBase_;
	public:
		virtual ~OptionsContainerBase_() = default;
	protected:
		// functions
		std::optional<int> Init_(bool captureDiagnostics) noexcept;
		CLI::Option* GetOpt_(class OptionsElementBase_&);
		virtual std::string GetName() const { return {}; }
		virtual std::string GetDesc() const { return {}; }
		// data
		std::ostringstream diagnostics_;
		bool finalized_ = false;
		CLI::App app_;
	};

	template<class T>
	class OptionsContainer : public OptionsContainerBase_
	{
	public:
		static T& Get()
		{
			auto& opts = Get_();
			chilass(opts.finalized_);
			return opts;
		}
		static bool IsInitialized()
		{
			return Get_().finalized_;
		}
		static std::string GetDiagnostics()
		{
			return Get_().diagnostics_.str();
		}
		static std::optional<int> Init(bool captureDiagnostics = true)
		{
			return Get_().Init_(captureDiagnostics);
		}
	private:
		static T& Get_()
		{
			static T opts;
			return opts;
		}
	};

	class OptionsElementBase_
	{
		friend OptionsContainerBase_;
		friend rule::RuleBase_;
	public:
		operator bool() const;
		bool operator!() const;
		std::string GetName() const;
	protected:
		// functions
		static CLI::App& GetApp_(OptionsContainerBase_* pContainer);
		// data
		CLI::Option* pOption_ = nullptr;
	};

	struct EmptyCustomizer {};

	template<typename T>
	class Option : public OptionsElementBase_
	{
	public:
		template<class C = EmptyCustomizer>
		Option(OptionsContainerBase_* pParent, std::string names, std::string description, std::optional<T> def = {}, const C& cust = EmptyCustomizer{})
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
		Option(const Option&) = delete;
		Option& operator=(const Option&) = delete;
		Option(Option&&) = delete;
		Option& operator=(Option&&) = delete;
		const T& operator*() const
		{
			return data_;
		}
		const T* operator->() const
		{
			return &data_;
		}
		std::optional<T> Opt() const
		{
			return *this ? std::optional<T>{ data_ } : std::nullopt;
		}
		T operator||(const T& rhs) const
		{
			return Opt().value_or(rhs);
		}
	private:
		T data_{};
	};

	class Flag : public OptionsElementBase_
	{
	public:
		Flag(OptionsContainerBase_* pParent, std::string names, std::string description);
		template<class C>
		Flag(OptionsContainerBase_* pParent, std::string names, std::string description, const C& cust)
			:
			Flag{ pParent, std::move(names), std::move(description) }
		{
			if constexpr (std::invocable<C, CLI::Option*>) {
				cust(pOption_);
			}
			else if constexpr (std::is_base_of_v<CLI::Validator, C>) {
				pOption_->transform(cust);
			}
		}
		Flag(const Flag&) = delete;
		Flag& operator=(const Flag&) = delete;
		Flag(Flag&&) = delete;
		Flag& operator=(Flag&&) = delete;
	private:
		bool data_{};
	};

	namespace rule
	{
		struct RuleBase_
		{
		protected:
			static CLI::Option* GetOption_(OptionsElementBase_& element)
			{
				return element.pOption_;
			}
			static CLI::App& GetApp_(OptionsContainerBase_* pParent)
			{
				return OptionsElementBase_::GetApp_(pParent);
			}
		};

		class MutualExclusion : public RuleBase_
		{
		public:
			template<class...T>
			MutualExclusion(T&...elements)
			{
				ExcludeRecursive_(elements...);
			}
		private:
			template<class T, class...Rest>
			void ExcludeRecursive_(T& pivot, Rest&...rest)
			{
				if constexpr (sizeof...(rest) > 0) {
					GetOption_(pivot)->excludes(GetOption_(rest)...);
					ExcludeRecursive_(rest...);
				}
			}
		};

		class Dependency : public RuleBase_
		{
		public:
			template<class Pivot, class...Dependents>
			Dependency(Pivot& pivot, Dependents&...dependents)
			{
				(DependencyImpl_(pivot, dependents), ...);
			}
		private:
			template<class Pivot, class Dependent>
			void DependencyImpl_(Pivot& pivot, Dependent& dependent)
			{
				GetOption_(pivot)->needs(GetOption_(dependent));
			}
		};

		class AllowExtras : public RuleBase_
		{
		public:
			AllowExtras(OptionsContainerBase_* pParent)
			{
				GetApp_(pParent).allow_extras(true);
			}
		};
	}

	template<typename E>
	CLI::Validator MakeEnumMap(bool ignoreCase = true)
	{
		using namespace magic_enum;
		auto pairs = vi::zip(enum_names<E>() | crn::Cast<std::string>(), enum_values<E>()) | rn::to<std::map>();
		if (ignoreCase) {
			return CLI::CheckedTransformer(std::move(pairs), CLI::ignore_case);
		}
		else {
			return CLI::CheckedTransformer(std::move(pairs));
		}
	}
}

#define CHIL_CLI_OPT(name, type, ...) ::chil::cli::Option<type> name{ this, ::chil::cli::OptionNameFromElementName(#name), __VA_ARGS__ }
#define CHIL_CLI_FLG(name, shortcut, ...) ::chil::cli::Flag name{ this, ::chil::cli::ComposeFlagName(#name, shortcut), __VA_ARGS__ }