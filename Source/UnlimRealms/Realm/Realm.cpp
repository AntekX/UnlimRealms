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
#include "Sys/Std/StdJobSystem.h"
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
		this->Deinitialize();
	}

	Result Realm::Initialize()
	{
		this->SetStorage( this->CreateDefaultStorage() );
		this->SetLog( this->CreateDefaultLog() );
		this->SetJobSystem( this->CreateDefaultJobSystem() );
		this->GetLog().WriteLine("Realm: initialized");
		return Result(Success);
	}

	Result Realm::Deinitialize()
	{
		// remove components before Log and/or Storage destroyed
		this->RemoveComponents();
		this->jobSystem.reset(ur_null);
		this->storage.reset(ur_null);
		if (this->log.get())
		{
			this->log->WriteLine("Realm: destroyed");
			this->log.reset(ur_null);
		}
		return Result(Success);
	}

	std::unique_ptr<Storage> Realm::CreateDefaultStorage()
	{
		std::unique_ptr<StdStorage> defaultStorage(new StdStorage(*this));
		return std::move(defaultStorage);
	}

	std::unique_ptr<Log> Realm::CreateDefaultLog()
	{
		std::unique_ptr<Log> defaultLog(new Log(*this, "unlim_log.txt"));
		defaultLog->SetPriority(Log::Note);
		return std::move(defaultLog);
	}

	std::unique_ptr<JobSystem> Realm::CreateDefaultJobSystem()
	{
		std::unique_ptr<StdJobSystem> defaultJobSystem(new StdJobSystem(*this));
		return std::move(defaultJobSystem);
	}

	Storage& Realm::GetStorage()
	{
		return *this->storage.get();
	}

	Log& Realm::GetLog()
	{
		return *this->log.get();
	}

	JobSystem& Realm::GetJobSystem()
	{
		return *this->jobSystem.get();
	}

	Input* Realm::GetInput() const
	{
		return this->GetComponent<Input>();
	}

	Canvas* Realm::GetCanvas() const
	{
		return this->GetComponent<Canvas>();
	}

	GfxSystem* Realm::GetGfxSystem() const
	{
		return this->GetComponent<GfxSystem>();
	}

	RealmEntity::RealmEntity(Realm &realm) :
		realm(realm)
	{
	}

	RealmEntity::~RealmEntity()
	{
	}
	
} // end namespace UnlimRealms