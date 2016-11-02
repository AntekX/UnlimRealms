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
		this->log->WriteLine("Realm: destroyed");
	}

	void Realm::CreateDefaultStorage()
	{
		this->storage.reset(new StdStorage(*this));
	}

	void Realm::CreateDefaultLog()
	{
		this->log.reset(new Log(*this, "unlim_log.txt"));
		this->log->SetPriority(Log::Note);
	}

	Result Realm::Initialize()
	{
		CreateDefaultStorage();
		CreateDefaultLog();
		this->log->WriteLine("Realm: initialized");
		return Result(Success);
	}

	void Realm::SetStorage(std::unique_ptr<Storage> storage)
	{
		this->storage = std::move(storage);
	}

	void Realm::SetInput(std::unique_ptr<Input> input)
	{
		this->input = std::move(input);
	}

	void Realm::SetLog(std::unique_ptr<Log> log)
	{
		this->log = std::move(log);
	}

	void Realm::SetCanvas(std::unique_ptr<Canvas> canvas)
	{
		this->canvas = std::move(canvas);
	}

	void Realm::SetGfxSystem(std::unique_ptr<GfxSystem> gfxSystem)
	{
		this->gfxSystem = std::move(gfxSystem);
	}

	RealmEntity::RealmEntity(Realm &realm) :
		realm(realm)
	{
	}

	RealmEntity::~RealmEntity()
	{
	}
	
} // end namespace UnlimRealms