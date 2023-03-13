#include "NoReturn.h"
#include "Assert.h"

namespace chil::utl
{
	void NoReturnImpl_()
	{
		chilass(false).msg(L"Reached no_return guard!");
	}
}