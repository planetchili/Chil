#include "ChilCppUnitTest.h"
#include <Core/src/ioc/Container.h>
#include <memory>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

struct Base
{
	virtual int Test() { return 420; }
	virtual ~Base() = default;
};

struct Derived : public Base
{
	int Test() override { return 69; }
};

struct ParameterizedClass
{
	struct IocParams
	{
		std::string s;
	};
	ParameterizedClass(IocParams params) : s{ std::move(params.s) } {}
	std::string s;
};

struct Dependant
{
	std::shared_ptr<Base> pDependency;
	Dependant(std::shared_ptr<Base> pDependency_in) : pDependency{ std::move(pDependency_in) } {}
};

using namespace chil;
using namespace std::string_literals;

namespace Ioc
{
	TEST_CLASS(IocContainerTests)
	{
	public:
		TEST_METHOD_INITIALIZE(Init)
		{
			pIoc = std::make_unique<ioc::Container>();
		}
		// registering a service with a derived implementation, and resolving derived
		TEST_METHOD(PolymorphicResolve)
		{
			pIoc->Register<Base>([] {return std::make_shared<Derived>(); });
			Assert::AreEqual(69, pIoc->Resolve<Base>()->Test());
		}
		// trying to resolve a service without registering
		TEST_METHOD(SimpleResolveFailure)
		{
			Assert::ExpectException<std::runtime_error>([this] {
				pIoc->Resolve<Base>();
			});
		}
		// parameterized resolve
		// (this test helped me find a mistake in ioc::Container::Resolve)
		TEST_METHOD(ParameterizedResolve)
		{
			pIoc->Register<ParameterizedClass>([](ParameterizedClass::IocParams p){
				return std::make_shared<ParameterizedClass>(std::move(p));
			});
			Assert::AreEqual("pube"s, pIoc->Resolve<ParameterizedClass>({.s = "pube"s})->s);
		}
		// cascaded resolve, where factories also use container to resolve dependencies
		TEST_METHOD(CascadedResolve)
		{
			pIoc->Register<Dependant>([this] {
				return std::make_shared<Dependant>(pIoc->Resolve<Base>());
			});
			pIoc->Register<Base>([this] {
				return std::make_shared<Base>();
			});
			Assert::AreEqual(420, pIoc->Resolve<Dependant>()->pDependency->Test());
		}
		// independent resolve, where resolved instances of the same type are separate objects
		TEST_METHOD(IndependentResolve)
		{
			pIoc->Register<ParameterizedClass>([](ParameterizedClass::IocParams p) {
				return std::make_shared<ParameterizedClass>(std::move(p));
			});
			auto pFirst = pIoc->Resolve<ParameterizedClass>({ .s = "first"s });
			auto pSecond = pIoc->Resolve<ParameterizedClass>({ .s = "second"s });
			Assert::AreEqual("first"s, pFirst->s);
			Assert::AreEqual("second"s, pSecond->s);
			// changing one should not affect the other
			pFirst->s = "pube"s;
			Assert::AreEqual("pube"s, pFirst->s);
			Assert::AreNotEqual("pube"s, pSecond->s);
		}		
		// replacing existing factory
		TEST_METHOD(ReplacementInjection)
		{
			// first make initial registration and check
			pIoc->Register<Dependant>([this] {
				return std::make_shared<Dependant>(pIoc->Resolve<Base>());
			});
			pIoc->Register<Base>([this] {
				return std::make_shared<Base>();
			});
			Assert::AreEqual(420, pIoc->Resolve<Dependant>()->pDependency->Test());
			// replace nested dependency and verify that Dependant with be affected when resolved
			pIoc->Register<Base>([this] {
				return std::make_shared<Derived>();
			});
			Assert::AreEqual(69, pIoc->Resolve<Dependant>()->pDependency->Test());
		}
	private:
		std::unique_ptr<ioc::Container> pIoc;
	};
}
