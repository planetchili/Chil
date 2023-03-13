#pragma once
#include <unordered_map>
#include <typeindex>
#include <any>
#include <functional>
#include <memory>
#include <stdexcept>
#include <format>
#include "Container.h"
#include "Exception.h"
#include <Core/src/utl/Assert.h>
#include <Core/src/utl/String.h>

namespace chil::ioc
{
	class Singletons
	{
	public:
		// types
		template<class T>
		using Generator = std::function<std::shared_ptr<T>()>;
		// functions
		template<class T>
		void Register(Generator<T> gen)
		{
			serviceMap_[typeid(T)] = gen;
		}
		template<class T>
		void RegisterPassthru()
		{
			Register<T>([] { return ioc::Get().Resolve<T>(); });
		}
		template<class T>
		std::shared_ptr<T> Resolve()
		{
			// TODO: pull this out of template/header
			if (const auto i = serviceMap_.find(typeid(T)); i != serviceMap_.end())
			{
				auto& entry = i->second;
				try {
					// first check if we have an existing instance, return if so
					if (auto ppInstance = std::any_cast<std::shared_ptr<T>>(&entry)) {
						return *ppInstance;
					}
					// if not, generate instance, store, and return
					auto pInstance = std::any_cast<Generator<T>>(entry)();
					entry = pInstance;
					return pInstance;
				}
				catch (const std::bad_any_cast&) {
					chilass(false).msg(std::format(
						L"Could not resolve Singleton mapped type\nfrom: [{}]\n  to: [{}]\n",
						utl::ToWide(entry.type().name()), utl::ToWide(typeid(Generator<T>).name())
					)).ex();

					no_return;
				}
			}
			else
			{
				throw ServiceNotFound{ std::format("Could not find entry for type [{}] in singleton container", typeid(T).name()) };
			}
		}
	private:
		// data
		std::unordered_map<std::type_index, std::any> serviceMap_;
	};

	Singletons& Sing();
}