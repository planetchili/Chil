#pragma once 
#include <string> 
#include "ChilWin.h"

namespace chil::win
{
	std::wstring GetErrorDescription(HRESULT hr);
}