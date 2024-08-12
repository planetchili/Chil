#pragma once
#include <optional>
#include <Core/src/utl/Assert.h>
#include <sstream>
#include <memory>
#include <functional>

namespace CLI
{
	class App;
	class Option;
}

namespace chil::cli
{
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
		OptionsContainerBase_();
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
		std::shared_ptr<CLI::App> pApp_;
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
		static T& Get_();
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

	namespace cust
	{
		class Customizer
		{
		public:
			Customizer(std::function<void(CLI::Option*)> f = [](CLI::Option* pOpt) {})
			{
				function_ = std::move(f);
			}
			Customizer operator|(const Customizer& other) const
			{
				return Customizer([f1 = function_, f2 = other.function_](CLI::Option* pOpt) {
					f1(pOpt);
					f2(pOpt);
				});
			}
			void Invoke(CLI::Option* pOpt) const
			{
				function_(pOpt);
			}
		private:
			std::function<void(CLI::Option*)> function_;
		};

		Customizer ExistingPath();
		Customizer ExistingDirectory();
		Customizer ExistingFile();
		Customizer NonExistingPath();
		Customizer Ipv4();
		Customizer NonNegative();
		Customizer Positive();
		template<typename T>
		Customizer Range(T min, T max);
		template<typename T>
		Customizer Clamp(T min, T max);
		template<bool IgnoreCase = true, typename T>
		Customizer In(std::initializer_list<T> values);
		template<bool IgnoreCase = true, typename T>
		Customizer In(T&& container);
		template<typename E>
		Customizer EnumMap(bool ignoreCase = true);
	}

	template<typename T>
	class Option : public OptionsElementBase_
	{
	public:
		Option(OptionsContainerBase_* pParent, std::string names, std::string description, std::optional<T> def = {}, const cust::Customizer& customizer = {});
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
		Flag(OptionsContainerBase_* pParent, std::string names, std::string description, const cust::Customizer& customizer = {});
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
			static CLI::Option* GetOption_(OptionsElementBase_& element);
			static CLI::App& GetApp_(OptionsContainerBase_* pParent);
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
			void ExcludeRecursive_(T& pivot, Rest&...rest);
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
			void DependencyImpl_(Pivot& pivot, Dependent& dependent);
		};

		class AllowExtras : public RuleBase_
		{
		public:
			AllowExtras(OptionsContainerBase_* pParent);
		};
	}
}

#define CHIL_CLI_OPT(name, type, ...) ::chil::cli::Option<type> name{ this, ::chil::cli::OptionNameFromElementName(#name), __VA_ARGS__ }
#define CHIL_CLI_FLG(name, shortcut, ...) ::chil::cli::Flag name{ this, ::chil::cli::ComposeFlagName(#name, shortcut), __VA_ARGS__ }