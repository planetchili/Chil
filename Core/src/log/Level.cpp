#include "Level.h"

namespace chil::log
{
	std::wstring GetLevelName(Level lv)
	{
		switch (lv)
		{
		case Level::Trace: return L"Trace";
		case Level::Debug: return L"Debug";
		case Level::Info: return L"Info";
		case Level::Warn: return L"Warning";
		case Level::Error: return L"Error";
		case Level::Fatal: return L"Fatal";
		default: return L"Unknown";
		}
	}
}