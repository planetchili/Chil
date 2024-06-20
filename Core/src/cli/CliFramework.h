#pragma once
#include <CLI/CLI.hpp>
#include <optional>
#include <Core/src/utl/Assert.h>


namespace chil::cli
{
	class OptionsContainerBase_
	{
		friend class OptionsElementBase_;
	protected:
		// functions
		std::optional<int> Init_() noexcept;
		// data
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
		static std::optional<int> Init()
		{
			return Get_().Init_();
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
		const T& operator*() const
		{
			return data_;
		}
	private:
		T data_{};
	};
}