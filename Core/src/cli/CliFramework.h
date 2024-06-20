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

	template<typename T>
	class Option : public OptionsElementBase_
	{
	public:
		Option(OptionsContainerBase_* pParent, std::string names, std::string description)
		{
			pOption_ = GetApp_(pParent).add_option(std::move(names), data_, std::move(description));
		}
		Option(const Option&) = delete;
		Option& operator=(const Option&) = delete;
		Option(Option&&) = delete;
		Option& operator=(Option&&) = delete;
		const T& operator*() const
		{
			return data_;
		}
	private:
		T data_{};
	};

	class Flag : public OptionsElementBase_
	{
	public:
		Flag(OptionsContainerBase_* pParent, std::string names, std::string description);
		Flag(const Flag&) = delete;
		Flag& operator=(const Flag&) = delete;
		Flag(Flag&&) = delete;
		Flag& operator=(Flag&&) = delete;
	private:
		bool data_{};
	};
}