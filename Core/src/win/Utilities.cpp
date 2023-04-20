#include "Utilities.h" 
#include <Core/src/log/Log.h>
#include "Exception.h"

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

	RECT ToWinRect(const spa::RectI& rSpa)
	{
		return {
			.left = rSpa.left,
			.top = rSpa.top,
			.right = rSpa.right,
			.bottom = rSpa.bottom,
		};
	}

	spa::RectI ToSpaRect(const RECT& rWin)
	{
		return {
			.left = rWin.left,
			.top = rWin.top,
			.right = rWin.right,
			.bottom = rWin.bottom,
		};
	}

	spa::DimensionsI ClientToWindowDimensions(const spa::DimensionsI& dims, DWORD styles)
	{
		using namespace spa;
		auto rect = ToWinRect(RectI::FromPointAndDimensions({ 0, 0 }, dims));
		if (AdjustWindowRect(&rect, styles, FALSE) == FALSE) {
			chilog.error(L"Failed to adjust window rect").hr();
			throw WindowException{};
		}
		return ToSpaRect(rect).GetDimensions();
	}
}