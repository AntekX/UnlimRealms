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
#include "Sys/Std/StdTaskManager.h"
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
		// remove components before Log and/or Storage destroyed
		this->RemoveComponents();
		this->GetLog().WriteLine("Realm: destroyed");
	}

	Result Realm::Initialize()
	{
		CreateDefaultStorage();
		CreateDefaultLog();
		this->GetLog().WriteLine("Realm: initialized");
		return Result(Success);
	}

	void Realm::CreateDefaultStorage()
	{
		std::unique_ptr<StdStorage> defaultStorage(new StdStorage(*this));
		this->storage = std::move(defaultStorage);
	}

	void Realm::CreateDefaultLog()
	{
		std::unique_ptr<Log> defaultLog(new Log(*this, "unlim_log.txt"));
		defaultLog->SetPriority(Log::Note);
		this->log = std::move(defaultLog);
	}

	void Realm::CreateDefaultTaskManager()
	{
		std::unique_ptr<StdTaskManager> defaultTaskManager(new StdTaskManager(*this));
		this->taskManager = std::move(defaultTaskManager);
	}

	Storage& Realm::GetStorage()
	{
		return *this->storage.get();
	}

	Log& Realm::GetLog()
	{
		return *this->log.get();
	}

	TaskManager& Realm::GetTaskManager()
	{
		return *this->taskManager.get();
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