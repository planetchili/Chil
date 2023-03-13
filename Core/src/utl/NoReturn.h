#pragma once  

namespace chil::utl
{
	[[noreturn]]
	void NoReturnImpl_();

#define no_return utl::NoReturnImpl_()
}