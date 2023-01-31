#include "Level.h"

namespace chil::log
{
	std::wstring GetLevelName(Level lv)
	{
		switch (lv)
		{
		case Level::Verbose: return L"Verbose";
		case Level::Debug: return L"Debug";
		case Level::Info: return L"Info";
		case Level::Warn: return L"Warning";
		case Level::Error: return L"Error";
		case Level::Fatal: return L"Fatal";
		default: return L"Unknown";
		}
	}
}