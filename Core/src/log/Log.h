#pragma once
#include "Channel.h" 
#include "EntryBuilder.h" 

namespace chil::log
{
	IChannel* GetDefaultChannel();

	void Boot();
}

#define chilog log::EntryBuilder{ __FILEW__, __FUNCTIONW__, __LINE__ }.chan(log::GetDefaultChannel())