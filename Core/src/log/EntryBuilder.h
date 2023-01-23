#pragma once
#include "Entry.h"

namespace chil::log
{
	class IChannel;

	class EntryBuilder : private Entry
	{
	public:
		EntryBuilder(const wchar_t* sourceFile, const wchar_t* sourceFunctionName, int sourceLine);
		EntryBuilder& note(std::wstring note);
		EntryBuilder& level(Level);
		EntryBuilder& chan(IChannel*);
		~EntryBuilder();
	private:
		IChannel* pDest_ = nullptr;
	};
}