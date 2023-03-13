#pragma once 
#include <exception> 
#include <string> 

namespace chil::utl
{
	class IException : public std::exception {};

	class BufferedException : public IException
	{
	public:
		BufferedException() = default;
		BufferedException(std::string msg);
		BufferedException(const std::wstring& msg);
		const char* what() const override;
	private:
		std::string message_;
		mutable std::string buffer_;
	};
}

#define ZC_EX_DEF_FROM(NewType, BaseType) class NewType : public BaseType {using Base = BaseType; public: using Base::Base;} 
#define ZC_EX_DEF(NewType) ZC_EX_DEF_FROM(NewType, chil::utl::BufferedException)