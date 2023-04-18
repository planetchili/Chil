#include "Utilities.h" 
#include <Core/src/log/Log.h>

namespace chil::win
{
	std::wstring GetErrorDescription(HRESULT hr)
	{
		wchar_t* descriptionWinalloc = nullptr;
		const auto result = FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPWSTR>(&descriptionWinalloc), 0, nullptr
		);

		std::wstring description;
		if (!result) {
			chilog.warn(L"Failed formatting windows error");
		}
		else {
			description = descriptionWinalloc;
			if (LocalFree(descriptionWinalloc)) {
				chilog.warn(L"Failed freeing memory for windows error formatting");
			}
			if (description.ends_with(L"\r\n")) {
				description.resize(description.size() - 2);
			}
		}
		return description;
	}
}