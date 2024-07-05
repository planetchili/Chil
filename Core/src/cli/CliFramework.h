#pragma once
#include <CLI/CLI.hpp>
#include <optional>
#include <Core/src/utl/Assert.h>
#include <sstream>


namespace chil::cli
{
	class OptionsContainerBase_
	{
		friend class OptionsElementBase_;
	protected:
		// functions
		std::optional<int> Init_(bool captureDiagnostics) noexcept;
		CLI::Option* GetOpt_(class OptionsElementBase_&);
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
	public:
		operator bool() const;
		bool operator!() const;
		std::string GetName() const;
	protected:
		// functions
		CLI::App& GetApp_(OptionsContainerBase_* pContainer);
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
}