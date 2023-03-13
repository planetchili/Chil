#include "Exception.h" 
#include "String.h" 
#include <format> 
#include <typeinfo> 

namespace chil::utl
{
	BufferedException::BufferedException(std::string msg)
		:
		message_{ std::move(msg) }
	{}
	BufferedException::BufferedException(const std::wstring& msg)
		:
		message_{ ToNarrow(msg) }
	{}
	const char* BufferedException::what() const
	{
		using namespace std::string_literals;
		buffer_ = "["s + typeid(const_cast<BufferedException&>(*this)).name() + "]"s;
		if (!message_.empty()) {
			buffer_ += ": ";
			buffer_ += message_;
		}
		return buffer_.c_str();
	}
}