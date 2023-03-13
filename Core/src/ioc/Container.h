#pragma once
#include <memory>
#include <unordered_map>
#include <functional>
#include <any>
#include <typeindex>
#include <stdexcept>
#include <format>
#include <tuple>
#include <stdexcept>
#include "Exception.h"
#include <Core/src/utl/Assert.h>
#include <Core/src/utl/String.h>


namespace chil::ioc
{
	template<class T>
	concept Parameterized = requires() {
		{ typename T::IocParams{} };
	};
	template<class T>
	concept NotParameterized = !Parameterized<T>;

	class Container
	{
	public:
		// types
		template<class T>
		using Generator = std::function<std::shared_ptr<T>()>;
		template<class T>
		using ParameterizedGenerator = std::function<std::shared_ptr<T>(typename T::IocParams)>;
		// functions
		template<Parameterized T>
		void Register(ParameterizedGenerator<T> gen)
		{
			serviceMap_[typeid(T)] = gen;
		}
		template<NotParameterized T>
		void Register(Generator<T> gen)
		{
			serviceMap_[typeid(T)] = gen;
		}
		template<Parameterized T>
		std::shared_ptr<T> Resolve(typename T::IocParams&& params = {}) const
		{
			return Resolve_<T, ParameterizedGenerator<T>>(std::forward<typename T::IocParams>(params));
		}
		template<NotParameterized T>
		std::shared_ptr<T> Resolve() const
		{
			return Resolve_<T, Generator<T>>();
		}
	private:
		// functions
		template<class T, class G, typename...Ps>
		std::shared_ptr<T> Resolve_(Ps&&...arg) const
		{
			// TODO: pull this out of template/header
			if (const auto i = serviceMap_.find(typeid(T)); i != serviceMap_.end())
			{
				const auto& entry = i->second;
				try {
					return std::any_cast<G>(entry)(std::forward<Ps>(arg)...);
				}
				catch (const std::bad_any_cast&) {
					chilass(false).msg(std::format(
						L"Could not resolve IoC mapped type\nfrom: [{}]\n  to: [{}]\n",
						utl::ToWide(entry.type().name()), utl::ToWide(typeid(G).name())
					)).ex();

					no_return;
				}
			}
			else
			{
				throw ServiceNotFound{ std::format("Could not find generator for type [{}] in IoC container", typeid(T).name()) };
			}
		}
		// data
		std::unordered_map<std::type_index, std::any> serviceMap_;
	};

	Container& Get() noexcept;
}