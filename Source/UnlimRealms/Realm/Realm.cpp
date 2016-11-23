///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Sys/Std/StdStorage.h"
#include "Sys/Log.h"
#include "Sys/Input.h"
#include "Sys/Canvas.h"
#include "Gfx/GfxSystem.h"

namespace UnlimRealms
{

	Realm::Realm()
	{
	}

	Realm::~Realm()
	{
		this->GetLog().WriteLine("Realm: destroyed");
	}

	void Realm::CreateDefaultStorage()
	{
		//this->AddComponent<Storage, StdStorage>(*this);
	}

	void Realm::CreateDefaultLog()
	{
		this->AddComponent<Log, Log>(*this, "unlim_log.txt");
		this->GetLog().SetPriority(Log::Note);
	}

	Result Realm::Initialize()
	{
		CreateDefaultStorage();
		CreateDefaultLog();
		this->GetLog().WriteLine("Realm: initialized");
		return Result(Success);
	}

	RealmEntity::RealmEntity(Realm &realm) :
		realm(realm)
	{
	}

	RealmEntity::~RealmEntity()
	{
	}
	
} // end namespace UnlimRealms