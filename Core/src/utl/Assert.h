#pragma once  
#include <string>  
#include <sstream>  
#include <optional>  
#include "StackTrace.h"  
#include <format>  
#include "Macro.h"  
#include "Exception.h"

namespace chil::utl
{
	ZC_EX_DEF(FailedAssertion);

	class Assertion
	{
	public:
		// types
		enum class Consequence
		{
			Log,
			Terminate,
			Exception,
		};
		// functions
		Assertion(std::wstring expression, const wchar_t* file, const wchar_t* function, int line, Consequence consequence = Consequence::Terminate);
		~Assertion();
		Assertion& msg(const std::wstring& message);
		template<typename T>
		Assertion& watch(T&& val, const wchar_t* name)
		{
			stream_ << L"   " << name << L" => " << std::forward<T>(val) << L"\n";
			return *this;
		}
		void ex();
	private:
		const wchar_t* file_;
		const wchar_t* function_;
		int line_ = -1;
		Consequence consequence_;
		std::wostringstream stream_;
	};
}

#ifndef ZC_CHILASS_ACTIVE  
#ifdef NDEBUG  
#define ZC_CHILASS_ACTIVE false  
#else  
#define ZC_CHILASS_ACTIVE true  
#endif  
#endif  

#define chilass(expr) (!ZC_CHILASS_ACTIVE || bool(expr)) ? void(0) : (void)utl::Assertion{ ZC_WSTR(expr), __FILEW__, __FUNCTIONW__, __LINE__ }  

#define ass_watch(expr) watch((expr), ZC_WSTR(expr))

#define chilchk(expr) bool(expr) ? void(0) : (void)utl::Assertion{ ZC_WSTR(expr), __FILEW__, __FUNCTIONW__, __LINE__, ZC_CHILASS_ACTIVE ? utl::Assertion::Consequence::Terminate : utl::Assertion::Consequence::Log }  