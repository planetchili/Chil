#include "EntryBuilder.h"
#include "Channel.h"

namespace chil::log
{
	EntryBuilder::EntryBuilder(const wchar_t* sourceFile, const wchar_t* sourceFunctionName, int sourceLine)
		:
		Entry{
			.sourceFile_ = sourceFile,
			.sourceFunctionName_ = sourceFunctionName,
			.sourceLine_ = sourceLine,
		}
	{}
	EntryBuilder& EntryBuilder::note(std::wstring note)
	{
		note_ = std::move(note);
		return *this;
	}
	EntryBuilder& EntryBuilder::level(Level level)
	{
		level_ = level;
		return *this;
	}
	EntryBuilder& EntryBuilder::chan(IChannel* pChan)
	{
		pDest_ = pChan;
		return *this;
	}
	EntryBuilder::~EntryBuilder()
	{
		if (pDest_) {
			pDest_->Submit(*this);
		}
	}
}